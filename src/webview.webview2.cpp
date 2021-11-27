#include <WebView2.h>
#include <eventtoken.h>
#include <lock.hpp>
#include <system_error>
#include <webview.hpp>
#include <wil/com.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <wrl.h>

using Microsoft::WRL::Callback;

namespace saucer
{
    struct window::impl
    {
        HWND hwnd;
    };

    struct webview::impl
    {
        wil::com_ptr<ICoreWebView2> webview_window;
        wil::com_ptr<ICoreWebView2Controller> webview_controller;

        bool js_ready = false;
        lockpp::lock<std::vector<LPCWSTR>> scripts_once;
        lockpp::lock<std::vector<LPCWSTR>> injected_scripts;
        lockpp::lock<std::vector<std::string>> scripts_on_done;
        lockpp::lock<std::vector<std::function<void()>>> call_on_initialize;

        EventRegistrationToken source_changed_token;
        EventRegistrationToken message_received_token;
        EventRegistrationToken navigation_completed_token;

        static WNDPROC original_wnd_proc;
        static LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    WNDPROC webview::impl::original_wnd_proc;
    LRESULT webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        const auto *thiz = reinterpret_cast<webview *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (thiz)
        {
            switch (msg)
            {
            case WM_SIZE:
                if (thiz->m_impl->webview_controller)
                {
                    int width = LOWORD(l_param);
                    int height = HIWORD(l_param);
                    thiz->m_impl->webview_controller->put_Bounds(RECT{0, 0, width, height});
                }
                break;
            }
        }

        return CallWindowProc(original_wnd_proc, hwnd, msg, w_param, l_param);
    }

    std::string narrow(const std::wstring &w_str)
    {
        auto sz = WideCharToMultiByte(65001, 0, w_str.c_str(), static_cast<int>(w_str.length()), nullptr, 0, nullptr, nullptr);
        if (!sz)
        {
            return {};
        }

        std::string out(sz, 0);
        WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, out.data(), sz, nullptr, nullptr);
        return out;
    }

    std::wstring widen(const std::string &str)
    {
        auto wsz = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);
        if (!wsz)
        {
            return {};
        }

        std::wstring out(wsz, 0);
        MultiByteToWideChar(65001, 0, str.c_str(), -1, out.data(), wsz);
        out.resize(wsz - 1);
        return out;
    }

    webview::~webview() = default;
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        impl::original_wnd_proc =
            reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window::m_impl->hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(impl::wnd_proc)));

        if (auto res = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); res != RPC_E_CHANGED_MODE)
        {
            std::wstring appdata;
            wil::GetEnvironmentVariableW(L"LOCALAPPDATA", appdata);
            appdata += L"\\MicrosoftEdge";

            //! Whoever chose to make the class names this fucking long should rot in hell.
            using create_webview = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
            using create_environment = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

            CreateCoreWebView2EnvironmentWithOptions(
                nullptr, appdata.c_str(), nullptr,
                Callback<create_environment>([this](HRESULT, ICoreWebView2Environment *env) {
                    return env->CreateCoreWebView2Controller(
                        window::m_impl->hwnd,
                        Callback<create_webview>([this](HRESULT, ICoreWebView2Controller *controller) {
                            m_impl->webview_controller = controller;
                            controller->get_CoreWebView2(&m_impl->webview_window);

                            RECT bounds;
                            GetClientRect(window::m_impl->hwnd, &bounds);
                            controller->put_Bounds(bounds);

                            wil::com_ptr<ICoreWebView2Settings> settings;
                            m_impl->webview_window->get_Settings(&settings);

                            auto new_settings = settings.try_query<ICoreWebView2Settings3>();
                            if (new_settings)
                            {
                                new_settings->put_AreBrowserAcceleratorKeysEnabled(false);
                            }

                            m_impl->webview_window->add_SourceChanged(
                                Callback<ICoreWebView2SourceChangedEventHandler>([this](ICoreWebView2 *, ICoreWebView2SourceChangedEventArgs *) {
                                    if (m_url_changed_callback)
                                    {
                                        m_url_changed_callback(get_url());
                                    }

                                    m_impl->js_ready = false;
                                    return S_OK;
                                }).Get(),
                                &m_impl->source_changed_token);

                            m_impl->webview_window->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>([this](ICoreWebView2 *,
                                                                                             ICoreWebView2WebMessageReceivedEventArgs *args) {
                                    LPWSTR message{};
                                    args->TryGetWebMessageAsString(&message);

                                    on_message(narrow(message));
                                    return S_OK;
                                }).Get(),
                                &m_impl->message_received_token);

                            m_impl->webview_window->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>([this](ICoreWebView2 *,
                                                                                              ICoreWebView2NavigationCompletedEventArgs *) {
                                    auto scripts = m_impl->scripts_on_done.write();
                                    for (const auto &script : *scripts)
                                    {
                                        run_java_script(script);
                                    }

                                    m_impl->js_ready = true;
                                    auto scripts_once = m_impl->scripts_once.write();
                                    for (const auto &id : *scripts_once)
                                    {
                                        m_impl->webview_window->RemoveScriptToExecuteOnDocumentCreated(id);
                                    }
                                    scripts_once->clear();
                                    return S_OK;
                                }).Get(),
                                &m_impl->navigation_completed_token);

                            inject(R"(
                                window.saucer = {
                                    async on_message(message) 
                                    {
                                        window.chrome.webview.postMessage(message);
                                    }
                                };
                            )",
                                   load_time_t::creation);

                            auto funcs = m_impl->call_on_initialize.write();
                            for (const auto &func : *funcs)
                            {
                                func();
                            }
                            funcs->clear();

                            return S_OK;
                        }).Get());
                }).Get());

            CoUninitialize();
        }
        else
        {
            throw std::system_error(static_cast<int>(res), std::system_category());
        }
    }

    void webview::set_url(const std::string &url)
    {
        if (!m_impl->webview_controller)
        {
            m_impl->call_on_initialize.write()->push_back([=]() { set_url(url); });
            return;
        }

        m_impl->webview_window->Navigate(widen(url).c_str());
    }

    std::string webview::get_url() const
    {
        wil::unique_cotaskmem_string url;
        m_impl->webview_window->get_Source(&url);

        return narrow(url.get());
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!m_impl->webview_controller)
        {
            m_impl->call_on_initialize.write()->push_back([=]() { set_context_menu(enabled); });
            return;
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);
        settings->put_AreDefaultContextMenusEnabled(enabled);
    }

    bool webview::get_context_menu() const
    {
        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);

        BOOL enabled{};
        settings->get_AreDefaultContextMenusEnabled(&enabled);

        return enabled;
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!m_impl->webview_controller)
        {
            m_impl->call_on_initialize.write()->push_back([=]() { run_java_script(java_script); });
            return;
        }

        if (!m_impl->js_ready)
        {
            m_impl->webview_window->AddScriptToExecuteOnDocumentCreated(
                widen(java_script).c_str(),
                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT, LPCWSTR id) {
                    m_impl->scripts_once.write()->push_back(id);
                    return S_OK;
                }).Get());
        }
        else
        {
            m_impl->webview_window->ExecuteScript(widen(java_script).c_str(), nullptr);
        }
    }

    void webview::inject(const std::string &java_script, [[maybe_unused]] const load_time_t &load_time)
    {
        if (load_time == load_time_t::creation)
        {
            if (!m_impl->webview_window)
            {
                m_impl->call_on_initialize.write()->push_back([=]() { inject(java_script, load_time); });
                return;
            }

            m_impl->webview_window->AddScriptToExecuteOnDocumentCreated(
                widen(java_script).c_str(),
                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT, LPCWSTR id) {
                    m_impl->injected_scripts.write()->push_back(id);
                    return S_OK;
                }).Get());
        }
        else
        {
            m_impl->scripts_on_done.write()->push_back(java_script);
        }
    }

    void webview::clear_scripts()
    {
        auto scripts = m_impl->injected_scripts.write();

        for (const auto &script : *scripts)
        {
            m_impl->webview_window->RemoveScriptToExecuteOnDocumentCreated(script);
        }

        scripts->clear();
        m_impl->scripts_on_done.write()->clear();
    }

    void webview::on_url_changed(const url_changed_callback_t &callback)
    {
        m_url_changed_callback = callback;
    }

    void webview::on_message(const std::string &message) {}
} // namespace saucer