#include "webview.hpp"
#include "window.win32.impl.hpp"
#include "webview.webview2.impl.hpp"

#include <regex>
#include <shlwapi.h>

namespace saucer
{
    using Microsoft::WRL::Callback;
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        impl::original_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window::m_impl->hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(impl::wnd_proc)));
        m_impl->init_webview(window::m_impl->hwnd);

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);
        settings->put_AreDevToolsEnabled(false);

        if (auto new_settings = settings.try_query<ICoreWebView2Settings3>(); new_settings)
        {
            new_settings->put_AreBrowserAcceleratorKeysEnabled(false);
        }

        m_impl->webview->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>([this](auto...) {
                                               on_url_changed(get_url());
                                               return S_OK;
                                           }).Get(),
                                           nullptr);

        m_impl->webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>([this](auto, auto *args) {
                                                    LPWSTR message{};
                                                    args->TryGetWebMessageAsString(&message);
                                                    on_message(window::m_impl->narrow(message));
                                                    return S_OK;
                                                }).Get(),
                                                nullptr);

        m_impl->webview->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>([this](auto...) {
                                                     for (const auto &script : m_impl->scripts_on_done)
                                                     {
                                                         run_java_script(script);
                                                     }
                                                     for (const auto &script : m_impl->scripts_once)
                                                     {
                                                         run_java_script(script);
                                                     }
                                                     m_impl->scripts_once.clear();
                                                     return S_OK;
                                                 }).Get(),
                                                 nullptr);

        inject(R"(
                            window.saucer = {
                                async on_message(message) 
                                {
                                    window.chrome.webview.postMessage(message);
                                }
                            };
                            window.saucer.on_message("js_ready");
                            )",
               load_time::creation);
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

        BOOL rtn{};
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

    bool webview::get_transparent() const
    {
        return window::m_impl->transparency_enabled;
    }

    bool webview::get_context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_context_menu(); });
        }

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_impl->webview->get_Settings(&settings);

        BOOL rtn{};
        settings->get_AreDefaultContextMenusEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    void webview::serve_embedded(const std::string &file)
    {
        set_url(std::string{impl::scheme_prefix} + file);
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

        if (enabled)
        {
            m_impl->webview->OpenDevToolsWindow();
        }
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

    void webview::set_transparent(bool enabled, bool blur)
    {
        auto webview_controller2 = m_impl->webview_controller.try_query<ICoreWebView2Controller2>();
        if (webview_controller2)
        {
            webview_controller2->put_DefaultBackgroundColor(enabled ? COREWEBVIEW2_COLOR{0, 0, 0, 0} : COREWEBVIEW2_COLOR{255, 255, 255, 255});
        }
        window::m_impl->enable_transparency(enabled, blur);
    }

    void webview::embed_files(std::map<const std::string, const embedded_file> &&files)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, files = std::move(files)]() mutable { return embed_files(std::move(files)); });
        }

        m_embedded_files.merge(files);

        if (!m_impl->resource_requested_token)
        {
            m_impl->resource_requested_token = EventRegistrationToken{};
            m_impl->webview->AddWebResourceRequestedFilter(std::wstring{std::wstring{impl::scheme_prefix_w} + L"*"}.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

            m_impl->webview->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](auto, auto *args) {
                                                          wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                                                          args->get_Request(&request);

                                                          auto webview2 = m_impl->webview.try_query<ICoreWebView2_2>();
                                                          if (webview2)
                                                          {
                                                              wil::com_ptr<ICoreWebView2Environment> env;
                                                              webview2->get_Environment(&env);

                                                              LPWSTR raw_url{};
                                                              request->get_Uri(&raw_url);
                                                              auto url = window::m_impl->narrow(raw_url);

                                                              // TODO(webview2): Windows does not seem to offer any methods that are *as easy* as QUri
                                                              std::smatch match;
                                                              std::regex_search(url, match, std::regex{R"(^(https:\/?\/?[^:\/\s]+(\/\w+)*\/[\w\-\.]+[^#?\s]+))"});

                                                              url = match[1];

                                                              if (url.size() > impl::scheme_prefix.size())
                                                              {
                                                                  url = url.substr(impl::scheme_prefix.size());

                                                                  if (m_embedded_files.count(url))
                                                                  {
                                                                      const auto &file = m_embedded_files.at(url);

                                                                      wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                                                                      wil::com_ptr<IStream> data = SHCreateMemStream(file.data, static_cast<UINT>(file.size));
                                                                      env->CreateWebResourceResponse(data.get(), 200, L"OK", window::m_impl->widen("Content-Type: " + file.mime).c_str(),
                                                                                                     &response);

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
                                                      &*m_impl->resource_requested_token);
        }
    }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_scripts(); });
        }

        for (const auto &script : m_impl->injected_scripts)
        {
            m_impl->webview->RemoveScriptToExecuteOnDocumentCreated(script);
        }
        m_impl->injected_scripts.clear();
        m_impl->scripts_on_done.clear();
    }

    void webview::clear_embedded()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();

        if (m_impl->resource_requested_token)
        {
            m_impl->webview->remove_WebResourceRequested(*m_impl->resource_requested_token);
            m_impl->webview->RemoveWebResourceRequestedFilter(L"https://saucer/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

            m_impl->resource_requested_token.reset();
        }
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
        }
        else
        {
            m_impl->webview->ExecuteScript(window::m_impl->widen(java_script).c_str(), nullptr);
        }
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return inject(java_script, load_time); });
        }

        if (load_time == load_time::creation)
        {
            m_impl->webview->AddScriptToExecuteOnDocumentCreated(window::m_impl->widen(java_script).c_str(),
                                                                 Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT, LPCWSTR id) {
                                                                     m_impl->injected_scripts.emplace_back(id);
                                                                     return S_OK;
                                                                 }).Get());
        }
        else
        {
            m_impl->scripts_on_done.emplace_back(java_script);
        }
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
