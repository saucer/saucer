#include "webview.hpp"
#include "webview.webview2.impl.hpp"

#include "requests.hpp"
#include "instantiate.hpp"

#include "utils.win32.hpp"
#include "window.win32.impl.hpp"

#include <cassert>
#include <filesystem>

#include <shlobj.h>
#include <fmt/core.h>
#include <fmt/xchar.h>

namespace saucer
{
    namespace fs = std::filesystem;

    webview::webview(const options &options) : window(options), m_impl(std::make_unique<impl>())
    {
        m_impl->overwrite_wnd_proc(window::m_impl->hwnd);

        window::m_impl->change_background = [&]()
        {
            if (!m_impl->controller)
            {
                return;
            }

            ComPtr<ICoreWebView2Controller2> controller;
            m_impl->controller->QueryInterface(IID_PPV_ARGS(&controller));

            if (!controller)
            {
                return;
            }

            auto [r, g, b, a] = window::m_impl->background;

            controller->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{
                static_cast<std::uint8_t>(a), //
                static_cast<std::uint8_t>(r), //
                static_cast<std::uint8_t>(g), //
                static_cast<std::uint8_t>(b)  //
            });
        };

        auto copy = options;

        if (options.persistent_cookies && options.storage_path.empty())
        {
            WCHAR appdata[MAX_PATH];
            SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, appdata);

            copy.storage_path = fs::path{appdata} / "saucer";
        }

        m_impl->create_webview(this, window::m_impl->hwnd, std::move(copy));

        // We disable the dev-tools explicitly as they're enabled by default on webview2.
        set_dev_tools(false);

        // We disable the Accelerator-Keys because they should be disabled by default.
        ComPtr<ICoreWebView2Settings> settings;
        m_impl->web_view->get_Settings(&settings);

        ComPtr<ICoreWebView2Settings3> settings3;
        settings->QueryInterface(IID_PPV_ARGS(&settings3));
        settings3->put_AreBrowserAcceleratorKeysEnabled(false);

        m_impl->web_view->add_WebMessageReceived(mcb{[this](auto, auto *args)
                                                     {
                                                         LPWSTR message{};
                                                         args->TryGetWebMessageAsString(&message);

                                                         on_message(utils::narrow(message));
                                                         CoTaskMemFree(message);

                                                         return S_OK;
                                                     }},
                                                 nullptr);

        m_impl->web_view->add_NavigationStarting(mcb{[this](auto...)
                                                     {
                                                         m_impl->dom_loaded = false;
                                                         m_events.at<web_event::load_started>().fire();

                                                         return S_OK;
                                                     }},
                                                 nullptr);

        // TODO: This is currently done to match the QWebEngineView behavior.
        // TODO: However we should eventually add an event to make it possible to handle this on your own (i.e. also
        // TODO: implement new-windows in the Qt backend).

        m_impl->web_view->add_NewWindowRequested(mcb{[](auto, auto *args)
                                                     {
                                                         args->put_Handled(true);

                                                         return S_OK;
                                                     }},
                                                 nullptr);

        ComPtr<ICoreWebView2_2> webview2_2;
        m_impl->web_view->QueryInterface(IID_PPV_ARGS(&webview2_2));

        webview2_2->add_DOMContentLoaded(mcb{[this](auto...)
                                             {
                                                 m_impl->dom_loaded = true;

                                                 for (const auto &script : m_impl->scripts)
                                                 {
                                                     execute(script);
                                                 }

                                                 for (const auto &pending : m_impl->pending)
                                                 {
                                                     execute(pending);
                                                 }

                                                 m_impl->pending.clear();
                                                 m_events.at<web_event::dom_ready>().fire();

                                                 return S_OK;
                                             }},
                                         nullptr);

        inject(impl::inject_script, load_time::creation);
    }

    webview::~webview() = default;

    bool webview::on_message(const std::string &message)
    {
        static constexpr auto opts = glz::opts{.error_on_unknown_keys = true, .error_on_missing_keys = true};

        request req;

        if (glz::read<opts>(req, message) != glz::error_code::none)
        {
            return false;
        }

        if (std::holds_alternative<resize_request>(req))
        {
            auto data = std::get<resize_request>(req);
            start_resize(static_cast<window_edge>(data.edge));

            return true;
        }

        if (std::holds_alternative<drag_request>(req))
        {
            start_drag();
            return true;
        }

        return false;
    }

    bool webview::dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return dev_tools(); });
        }

        ComPtr<ICoreWebView2Settings> settings;
        m_impl->web_view->get_Settings(&settings);

        BOOL rtn{false};
        settings->get_AreDevToolsEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    std::string webview::url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return url(); });
        }

        LPWSTR url{};
        m_impl->web_view->get_Source(&url);

        auto rtn = utils::narrow(url);
        CoTaskMemFree(url);

        return rtn;
    }

    bool webview::context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return context_menu(); });
        }

        ComPtr<ICoreWebView2Settings> settings;
        m_impl->web_view->get_Settings(&settings);

        BOOL rtn{false};
        settings->get_AreDefaultContextMenusEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, enabled] { return set_dev_tools(enabled); });
        }

        ComPtr<ICoreWebView2Settings> settings;
        m_impl->web_view->get_Settings(&settings);

        settings->put_AreDevToolsEnabled(enabled);

        if (!enabled)
        {
            return;
        }

        m_impl->web_view->OpenDevToolsWindow();
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, enabled] { return set_context_menu(enabled); });
        }

        ComPtr<ICoreWebView2Settings> settings;
        m_impl->web_view->get_Settings(&settings);

        settings->put_AreDefaultContextMenusEnabled(enabled);
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, url] { return set_url(url); });
        }

        m_impl->web_view->Navigate(utils::widen(url).c_str());
    }

    void webview::embed(embedded_files &&files)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, files = std::move(files)]() mutable
                                             { return embed(std::move(files)); });
        }

        m_embedded_files.merge(files);

        if (m_impl->scheme_handler.value > 0)
        {
            return;
        }

        m_impl->install_scheme_handler(this);
    }

    void webview::serve(const std::string &file)
    {
        set_url(std::string{impl::scheme_prefix} + file);
    }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_scripts(); });
        }

        for (const auto &script : m_impl->injected)
        {
            m_impl->web_view->RemoveScriptToExecuteOnDocumentCreated(script);
        }

        m_impl->injected.clear();
    }

    void webview::clear_embedded()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();

        if (m_impl->scheme_handler.value <= 0)
        {
            return;
        }

        m_impl->web_view->remove_WebResourceRequested(m_impl->scheme_handler);

        static const auto uri = fmt::format(L"{}*", utils::widen(std::string{impl::scheme_prefix}));
        m_impl->web_view->RemoveWebResourceRequestedFilter(uri.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

        m_impl->scheme_handler = {};
    }

    void webview::execute(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, java_script] { return execute(java_script); });
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(java_script);
            return;
        }

        if (!SUCCEEDED(m_impl->web_view->ExecuteScript(utils::widen(java_script).c_str(), nullptr)))
        {
            assert("Failed to execute script" && false);
        }
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (java_script.empty())
        {
            return;
        }

        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, java_script, load_time] { return inject(java_script, load_time); });
        }

        if (load_time == load_time::ready)
        {
            m_impl->scripts.emplace_back(java_script);
            return;
        }

        auto cb = mcb{[this](auto, LPCWSTR id)
                      {
                          m_impl->injected.emplace_back(id);
                          return S_OK;
                      }};

        auto status = m_impl->web_view->AddScriptToExecuteOnDocumentCreated(utils::widen(java_script).c_str(), cb);

        if (!SUCCEEDED(status))
        {
            assert("Failed to inject script" && false);
        }
    }

    void webview::clear(web_event event)
    {
        switch (event)
        {
        case web_event::load_finished:
            m_impl->web_view->remove_NavigationCompleted(m_impl->load_finished);
            m_impl->load_finished = {};
            break;
        case web_event::url_changed:
            m_impl->web_view->remove_SourceChanged(m_impl->url_changed);
            m_impl->url_changed = {};
            break;
        default:
            break;
        };

        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <web_event Event>
    void webview::once(events::type_t<Event> &&callback)
    {
        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type_t<Event> &&callback)
    {
        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    INSTANTIATE_EVENTS(webview, 4, web_event)
} // namespace saucer
