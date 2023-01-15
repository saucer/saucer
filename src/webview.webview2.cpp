#include "webview.hpp"
#include "window.win32.impl.hpp"
#include "webview.webview2.impl.hpp"

#include <wil/win32_helpers.h>
#include <filesystem>
#include <regex>
#include <wrl.h>

namespace saucer
{
    namespace fs = std::filesystem;
    using Microsoft::WRL::Callback;

    webview::webview(const webview_options &options) : m_impl(std::make_unique<impl>())
    {
        m_impl->overwrite_wnd_proc(window::m_impl->hwnd);

        window::m_impl->change_background = [&]() {
            if (!m_impl->controller)
            {
                return;
            }

            auto controller2 = m_impl->controller.try_query<ICoreWebView2Controller2>();

            if (!controller2)
            {
                return;
            }

            auto [r, g, b, a] = window::m_impl->background_color;

            controller2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{
                static_cast<std::uint8_t>(a), //
                static_cast<std::uint8_t>(r), //
                static_cast<std::uint8_t>(g), //
                static_cast<std::uint8_t>(b)  //
            });
        };

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        std::wstring user_folder;
        wil::GetEnvironmentVariableW(L"TEMP", user_folder);

        if (options.persistent_cookies)
        {
            std::wstring appdata;
            wil::GetEnvironmentVariableW(L"APPDATA", appdata);

            auto path = fs::path{appdata} / "saucer";

            if (!fs::exists(path))
            {
                fs::create_directories(path);
            }

            user_folder = path.wstring();
        }

        //? Ensure the WebView is created synchronously, may be easier in the future:
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/740
        m_impl->create_webview(window::m_impl->hwnd, user_folder);

        while (!m_impl->webview)
        {
            window::run<false>();
        }

        // ? We disable the dev-tools explicitly as they're enabled by default on webview2.
        set_dev_tools(false);

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);

        // ? We disable the Accelerator-Keys because they should be disabled by default
        if (auto settings3 = settings.try_query<ICoreWebView2Settings3>(); settings3)
        {
            settings3->put_AreBrowserAcceleratorKeysEnabled(false);
        }

        using source_changed = ICoreWebView2SourceChangedEventHandler;
        auto url_changed = [&](auto...) { on_url_changed(get_url()); };
        m_impl->webview->add_SourceChanged(Callback<source_changed>(url_changed).Get(), nullptr);

        using received_event = ICoreWebView2WebMessageReceivedEventHandler;
        auto message_received = [&](auto, ICoreWebView2WebMessageReceivedEventArgs *args) {
            LPWSTR message{};
            args->TryGetWebMessageAsString(&message);

            on_message(window::m_impl->narrow(message));
            return S_OK;
        };
        m_impl->webview->add_WebMessageReceived(Callback<received_event>(message_received).Get(), nullptr);

        using navigation_event = ICoreWebView2NavigationCompletedEventHandler;
        auto navigation_completed = [&](auto...) {
            for (const auto &script : m_impl->scripts_load)
            {
                run_java_script(script);
            }

            for (const auto &script : m_impl->scripts_once)
            {
                run_java_script(script);
            }

            m_impl->scripts_once.clear();
            return S_OK;
        };
        m_impl->webview->add_NavigationCompleted(Callback<navigation_event>(navigation_completed).Get(), nullptr);

        inject(impl::inject_script, load_time::creation);

        CoUninitialize();
    }

    webview::~webview() = default;

    void webview::on_message(const std::string &message)
    {
        if (message == "js_ready")
        {
            m_impl->is_ready = true;
        }
    }

    void webview::on_url_changed(const std::string &url)
    {
        m_events.at<web_event::url_changed>().fire(url);
    }

    bool webview::get_dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_dev_tools(); });
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);

        BOOL rtn{false};
        settings->get_AreDevToolsEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    std::string webview::get_url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_url(); });
        }

        wil::unique_cotaskmem_string url;
        m_impl->webview->get_Source(&url);

        return window::m_impl->narrow(url.get());
    }

    bool webview::get_context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_context_menu(); });
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);

        BOOL rtn{false};
        settings->get_AreDefaultContextMenusEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_dev_tools(enabled); });
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);
        settings->put_AreDevToolsEnabled(enabled);

        if (!enabled)
        {
            return;
        }

        m_impl->webview->OpenDevToolsWindow();
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_context_menu(enabled); });
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);

        settings->put_AreDefaultContextMenusEnabled(enabled);
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_url(url); });
        }

        m_impl->webview->Navigate(window::m_impl->widen(url).c_str());
    }

    void webview::serve_embedded(const std::string &file)
    {
        set_url(std::string{impl::scheme_prefix} + file);
    }

    // void webview::embed_files(std::map<const std::string, const embedded_file> &&files)
    // {
    // TODO: Make beautiful
    //     if (!window::m_impl->is_thread_safe())
    //     {
    //         return window::m_impl->post_safe(
    //             [this, files = std::move(files)]() mutable { return embed_files(std::move(files)); });
    //     }

    //     m_embedded_files.merge(files);

    //     if (!m_impl->event_token)
    //     {
    //         m_impl->event_token = EventRegistrationToken{};
    //         m_impl->webview->AddWebResourceRequestedFilter(
    //             std::wstring{std::wstring{impl::scheme_prefix_w} + L"*"}.c_str(),
    //             COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

    //         m_impl->webview->add_WebResourceRequested(
    //             Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](auto, auto *args) {
    //                 wil::com_ptr<ICoreWebView2WebResourceRequest> request;
    //                 args->get_Request(&request);

    //                 auto webview2 = m_impl->webview.try_query<ICoreWebView2_2>();
    //                 if (webview2)
    //                 {
    //                     wil::com_ptr<ICoreWebView2Environment> env;
    //                     webview2->get_Environment(&env);

    //                     LPWSTR raw_url{};
    //                     request->get_Uri(&raw_url);
    //                     auto url = window::m_impl->narrow(raw_url);

    //                     // TODO(webview2): Windows does not seem to offer any methods that are *as easy* as QUri
    //                     std::smatch match;
    //                     std::regex_search(url, match,
    //                                       std::regex{R"(^(https:\/?\/?[^:\/\s]+(\/\w+)*\/[\w\-\.]+[^#?\s]+))"});

    //                     url = match[1];

    //                     if (url.size() > impl::scheme_prefix.size())
    //                     {
    //                         url = url.substr(impl::scheme_prefix.size());

    //                         if (m_embedded_files.count(url))
    //                         {
    //                             const auto &file = m_embedded_files.at(url);

    //                             wil::com_ptr<ICoreWebView2WebResourceResponse> response;
    //                             wil::com_ptr<IStream> data = SHCreateMemStream(file.data,
    //                             static_cast<UINT>(file.size)); env->CreateWebResourceResponse(
    //                                 data.get(), 200, L"OK", window::m_impl->widen("Content-Type: " +
    //                                 file.mime).c_str(), &response);

    //                             args->put_Response(response.get());
    //                         }
    //                         else
    //                         {
    //                             wil::com_ptr<ICoreWebView2WebResourceResponse> response;
    //                             env->CreateWebResourceResponse(nullptr, 404, L"Not found", L"", &response);

    //                             args->put_Response(response.get());
    //                         }
    //                     }
    //                     else
    //                     {
    //                         wil::com_ptr<ICoreWebView2WebResourceResponse> response;
    //                         env->CreateWebResourceResponse(nullptr, 500, L"Bad request", L"", &response);

    //                         args->put_Response(response.get());
    //                     }
    //                 }

    //                 return S_OK;
    //             }).Get(),
    //             &*m_impl->event_token);
    //     }
    // }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_scripts(); });
        }

        for (const auto &script : m_impl->injected)
        {
            m_impl->webview->RemoveScriptToExecuteOnDocumentCreated(script);
        }

        m_impl->injected.clear();
        m_impl->scripts_load.clear();
    }

    void webview::clear_embedded()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();

        if (!m_impl->event_token)
        {
            return;
        }

        // NOLINTNEXTLINE
        m_impl->webview->remove_WebResourceRequested(*m_impl->event_token);
        m_impl->webview->RemoveWebResourceRequestedFilter(L"https://saucer/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

        m_impl->event_token.reset();
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return run_java_script(java_script); });
        }

        if (!m_impl->is_ready)
        {
            m_impl->scripts_once.emplace_back(java_script);
            return;
        }

        m_impl->webview->ExecuteScript(window::m_impl->widen(java_script).c_str(), nullptr);
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return inject(java_script, load_time); });
        }

        if (load_time == load_time::creation)
        {
            // TODO: Make beautiful
            m_impl->webview->AddScriptToExecuteOnDocumentCreated(
                window::m_impl->widen(java_script).c_str(),
                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                    [this](HRESULT, LPCWSTR id) {
                        m_impl->injected.emplace_back(id);
                        return S_OK;
                    })
                    .Get());
            return;
        }

        m_impl->scripts_load.emplace_back(java_script);
    }

    void webview::clear(web_event event)
    {
        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <> std::uint64_t webview::on<web_event::url_changed>(events::callback_t<web_event::url_changed> &&callback)
    {
        return m_events.at<web_event::url_changed>().add(std::move(callback));
    }
} // namespace saucer
