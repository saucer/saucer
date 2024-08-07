#include "webview.webkit.impl.hpp"

#include "icon.gtk.impl.hpp"
#include "window.gtk.impl.hpp"

#include "requests.hpp"

#include <fmt/core.h>
#include <webkit/WebKitUserContentManager.h>

namespace saucer
{
    webview::webview(const options &options) : window(options), m_impl(std::make_unique<impl>())
    {
        m_impl->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->web_view), 1, 1);

        gtk_widget_set_vexpand(GTK_WIDGET(m_impl->web_view), true);
        gtk_widget_set_hexpand(GTK_WIDGET(m_impl->web_view), true);

        gtk_box_append(window::m_impl->content, GTK_WIDGET(m_impl->web_view));

        g_signal_connect(m_impl->web_view, "context-menu",
                         G_CALLBACK(+[](WebKitWebView *, WebKitContextMenu *, WebKitHitTestResult *, gpointer data)
                                    { return !reinterpret_cast<impl *>(data)->context_menu; }),
                         m_impl.get());

        auto *manager = webkit_web_view_get_user_content_manager(m_impl->web_view);
        webkit_user_content_manager_register_script_message_handler(manager, "saucer", nullptr);

        g_signal_connect(manager, "script-message-received",
                         G_CALLBACK(+[](WebKitWebView *, JSCValue *message, gpointer data)
                                    {
                                        auto *str = jsc_value_to_string(message);
                                        reinterpret_cast<webview *>(data)->on_message(str);
                                        g_free(str);
                                    }),
                         this);

        inject(impl::inject_script(), load_time::creation);
    }

    webview::~webview() = default;

    bool webview::on_message(const std::string &message)
    {
        auto request = requests::parse(message);

        if (!request)
        {
            return false;
        }

        if (std::holds_alternative<requests::resize>(request.value()))
        {
            const auto data = std::get<requests::resize>(request.value());
            start_resize(static_cast<window_edge>(data.edge));

            return true;
        }

        if (std::holds_alternative<requests::drag>(request.value()))
        {
            start_drag();
            return true;
        }

        return false;
    }

    icon webview::favicon() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return favicon(); }).get();
        }

        return {{object_ptr<GdkTexture>::copy(webkit_web_view_get_favicon(m_impl->web_view))}};
    }

    std::string webview::page_title() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return page_title(); }).get();
        }

        return webkit_web_view_get_title(m_impl->web_view);
    }

    bool webview::dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return dev_tools(); }).get();
        }

        auto *settings = webkit_web_view_get_settings(m_impl->web_view);
        return webkit_settings_get_enable_developer_extras(settings);
    }

    std::string webview::url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return url(); }).get();
        }

        const auto *rtn = webkit_web_view_get_uri(m_impl->web_view);

        if (!rtn)
        {
            return {};
        }

        return rtn;
    }

    bool webview::context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return context_menu(); }).get();
        }

        return m_impl->context_menu;
    }

    color webview::background() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return background(); }).get();
        }

        GdkRGBA color;
        webkit_web_view_get_background_color(m_impl->web_view, &color);

        return {
            static_cast<std::uint8_t>(color.red),
            static_cast<std::uint8_t>(color.green),
            static_cast<std::uint8_t>(color.blue),
            static_cast<std::uint8_t>(color.alpha),
        };
    }

    bool webview::force_dark_mode() const // NOLINT
    {
        // TODO: Is this possible?
        return {};
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_dev_tools(enabled); }).get();
        }

        auto *settings = webkit_web_view_get_settings(m_impl->web_view);
        webkit_settings_set_enable_developer_extras(settings, enabled);

        auto *inspector = webkit_web_view_get_inspector(m_impl->web_view);
        webkit_web_inspector_show(inspector);
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_context_menu(enabled); }).get();
        }

        m_impl->context_menu = enabled;
    }

    void webview::set_force_dark_mode(bool)
    {
        // TODO: Is this possible?
    }

    void webview::set_background(const color &background)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, background] { return set_background(background); }).get();
        }

        auto [r, g, b, a] = background;

        GdkRGBA color{
            .red   = static_cast<float>(r),
            .green = static_cast<float>(g),
            .blue  = static_cast<float>(b),
            .alpha = static_cast<float>(a),
        };

        webkit_web_view_set_background_color(m_impl->web_view, &color);
    }

    void webview::set_file(const fs::path &file)
    {
        auto path = fmt::format("file://{}", fs::canonical(file).string());
        set_url(path);
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, url]() { return set_url(url); }).get();
        }

        webkit_web_view_load_uri(m_impl->web_view, url.c_str());
    }

    // TODO: Embed, Serve, Clear-Scripts...

    void webview::execute(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, java_script]() { return execute(java_script); }).get();
        }

        // TODO: Check if dom is ready...

        webkit_web_view_evaluate_javascript(m_impl->web_view, java_script.c_str(), -1, nullptr, nullptr, nullptr,
                                            nullptr, nullptr);
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        auto *manager = webkit_web_view_get_user_content_manager(m_impl->web_view);
        auto *script =
            webkit_user_script_new(java_script.c_str(), WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                   load_time == load_time::creation ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
                                                                    : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END,
                                   nullptr, nullptr);

        m_impl->scripts.emplace_back(script);
        webkit_user_content_manager_add_script(manager, script);
    }
} // namespace saucer
