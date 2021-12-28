#include "utils.win32.hpp"
#include <Shlwapi.h>
#include <WebView2.h>
#include <future>
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
    using safe_call = std::function<void()>;

    struct window::impl
    {
        HWND hwnd;
        static UINT WM_SAFE_CALL;
        bool is_thread_safe() const;
    };

    struct webview::impl
    {
        wil::com_ptr<ICoreWebView2> webview_window;
        wil::com_ptr<ICoreWebView2Controller> webview_controller;

        bool js_ready = false;
        lockpp::lock<std::vector<LPCWSTR>> injected_scripts;
        lockpp::lock<std::vector<std::string>> scripts_once;
        lockpp::lock<std::vector<std::string>> scripts_on_done;

        EventRegistrationToken source_changed_token;
        EventRegistrationToken message_received_token;
        EventRegistrationToken navigation_completed_token;
        std::unique_ptr<EventRegistrationToken> resource_requested_token;

        static WNDPROC original_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    WNDPROC webview::impl::original_wnd_proc;
    LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        const auto *thiz = reinterpret_cast<webview *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

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
            case WM_SHOWWINDOW:
                thiz->m_impl->webview_controller->put_IsVisible(static_cast<BOOL>(w_param));
                break;
            }
        }

        return CallWindowProcW(original_wnd_proc, hwnd, msg, w_param, l_param);
    }

    webview::~webview() = default;
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        impl::original_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window::m_impl->hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(impl::wnd_proc)));

        if (auto res = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); res != RPC_E_CHANGED_MODE)
        {
            std::wstring appdata;
            wil::GetEnvironmentVariableW(L"LOCALAPPDATA", appdata);
            appdata += L"\\MicrosoftEdge";

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
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>([this](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) {
                                    LPWSTR message{};
                                    args->TryGetWebMessageAsString(&message);

                                    on_message(utils::narrow(message));
                                    return S_OK;
                                }).Get(),
                                &m_impl->message_received_token);

                            m_impl->webview_window->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>([this](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    auto scripts = m_impl->scripts_on_done.write();
                                    for (const auto &script : *scripts)
                                    {
                                        run_java_script(script);
                                    }

                                    m_impl->js_ready = true;

                                    auto scripts_once = m_impl->scripts_once.write();
                                    for (const auto &script : *scripts_once)
                                    {
                                        run_java_script(script);
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

                            return S_OK;
                        }).Get());
                }).Get());

            MSG msg; //? Ensure the WebView is created synchronously
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (m_impl->webview_controller)
                    break;
            }

            CoUninitialize();
        }
        else
        {
            throw std::system_error(static_cast<int>(res), std::system_category());
        }
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { set_url(url); })));
            return;
        }

        m_impl->webview_window->Navigate(utils::widen(url).c_str());
    }

    std::string webview::get_url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            std::promise<std::string> result;
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([&result, this] { result.set_value(get_url()); })));
            return result.get_future().get();
        }

        wil::unique_cotaskmem_string url;
        m_impl->webview_window->get_Source(&url);

        return utils::narrow(url.get());
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { set_dev_tools(enabled); })));
            return;
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);
        settings->put_AreDevToolsEnabled(enabled);
    }

    bool webview::get_dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            std::promise<bool> result;
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([&result, this] { result.set_value(get_dev_tools()); })));
            return result.get_future().get();
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);

        BOOL result{};
        settings->get_AreDevToolsEnabled(&result);

        return result;
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { set_context_menu(enabled); })));
            return;
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);
        settings->put_AreDefaultContextMenusEnabled(enabled);
    }

    bool webview::get_context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            std::promise<bool> result;
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([&result, this] { result.set_value(get_context_menu()); })));
            return result.get_future().get();
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview_window->get_Settings(&settings);

        BOOL enabled{};
        settings->get_AreDefaultContextMenusEnabled(&enabled);

        return enabled;
    }

    void webview::embed_files(const embedded_files &files)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { embed_files(files); })));
            return;
        }

        m_embedded_files = files;
        if (!m_impl->resource_requested_token)
        {
            m_impl->resource_requested_token = std::make_unique<EventRegistrationToken>();
            m_impl->webview_window->AddWebResourceRequestedFilter(L"https://saucer/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

            m_impl->webview_window->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](ICoreWebView2 *, ICoreWebView2WebResourceRequestedEventArgs *args) {
                    wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                    args->get_Request(&request);

                    auto webview_2 = m_impl->webview_window.try_query<ICoreWebView2_2>();
                    if (webview_2)
                    {
                        wil::com_ptr<ICoreWebView2Environment> env;
                        webview_2->get_Environment(&env);

                        LPWSTR raw_url{};
                        request->get_Uri(&raw_url);
                        auto url = utils::narrow(raw_url);

                        if (url.size() > 15)
                        {
                            url = url.substr(15);

                            if (m_embedded_files.count(url))
                            {
                                const auto &file = m_embedded_files.at(url);

                                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                                wil::com_ptr<IStream> data = SHCreateMemStream(std::get<2>(file), static_cast<UINT>(std::get<1>(file)));
                                env->CreateWebResourceResponse(data.get(), 200, L"OK", utils::widen("Content-Type: " + std::get<0>(file)).c_str(), &response);

                                args->put_Response(response.get());
                            }
                            else
                            {
                                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                                env->CreateWebResourceResponse(nullptr, 404, L"Not found", L"", &response);

                                args->put_Response(response.get());
                            }
                        }
                        else
                        {
                            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                            env->CreateWebResourceResponse(nullptr, 500, L"Bad request", L"", &response);

                            args->put_Response(response.get());
                        }
                    }

                    return S_OK;
                }).Get(),
                m_impl->resource_requested_token.get());
        }
    }

    void webview::clear_embedded()
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { clear_embedded(); })));
            return;
        }

        m_embedded_files.clear();

        if (m_impl->resource_requested_token)
        {
            m_impl->webview_window->remove_WebResourceRequested(*m_impl->resource_requested_token);
            m_impl->webview_window->RemoveWebResourceRequestedFilter(L"https://saucer/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

            m_impl->resource_requested_token.reset();
        }
    }

    void webview::serve_embedded(const std::string &file)
    {
        set_url("https://saucer/" + file);
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { run_java_script(java_script); })));
            return;
        }

        if (!m_impl->js_ready)
        {
            m_impl->scripts_once.write()->emplace_back(java_script);
        }
        else
        {
            m_impl->webview_window->ExecuteScript(utils::widen(java_script).c_str(), nullptr);
        }
    }

    void webview::inject(const std::string &java_script, [[maybe_unused]] const load_time_t &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { inject(java_script, load_time); })));
            return;
        }

        if (load_time == load_time_t::creation)
        {
            m_impl->webview_window->AddScriptToExecuteOnDocumentCreated(
                utils::widen(java_script).c_str(), Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT, LPCWSTR id) {
                                                       m_impl->injected_scripts.write()->emplace_back(id);
                                                       return S_OK;
                                                   }).Get());
        }
        else
        {
            m_impl->scripts_on_done.write()->emplace_back(java_script);
        }
    }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            PostMessageW(window::m_impl->hwnd, window::m_impl->WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new safe_call([=] { clear_scripts(); })));
            return;
        }

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