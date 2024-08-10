#include "webkit.webview.impl.hpp"

#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"
#include "webkit.scheme.impl.hpp"

#include "requests.hpp"
#include "instantiate.hpp"

#include <fmt/core.h>

namespace saucer
{
    webview::webview(const options &options) : window(options), m_impl(std::make_unique<impl>())
    {
        m_impl->web_view = g_object_ptr<WebKitWebView>::copy(WEBKIT_WEB_VIEW(webkit_web_view_new()));
        m_impl->settings = impl::make_settings(options);

        if (options.persistent_cookies)
        {
            auto *session = webkit_web_view_get_network_session(m_impl->web_view.get());
            auto *manager = webkit_network_session_get_cookie_manager(session);

            auto path = options.storage_path.empty() ? (fs::temp_directory_path() / "saucer") : options.storage_path;
            webkit_cookie_manager_set_persistent_storage(manager, path.c_str(),
                                                         WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
        }

        webkit_settings_set_hardware_acceleration_policy(
            m_impl->settings.get(), options.hardware_acceleration ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
                                                                  : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

        webkit_web_view_set_settings(m_impl->web_view.get(), m_impl->settings.get());

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->web_view.get()), 1, 1);
        gtk_widget_set_vexpand(GTK_WIDGET(m_impl->web_view.get()), true);
        gtk_widget_set_hexpand(GTK_WIDGET(m_impl->web_view.get()), true);

        gtk_box_append(window::m_impl->content, GTK_WIDGET(m_impl->web_view.get()));

        auto on_context = [](WebKitWebView *, WebKitContextMenu *, WebKitHitTestResult *, void *data)
        {
            return !reinterpret_cast<impl *>(data)->context_menu;
        };

        g_signal_connect(m_impl->web_view.get(), "context-menu", G_CALLBACK(+on_context), m_impl.get());

        m_impl->manager = webkit_web_view_get_user_content_manager(m_impl->web_view.get());
        webkit_user_content_manager_register_script_message_handler(m_impl->manager, "saucer", nullptr);

        auto on_message = [](WebKitWebView *, JSCValue *value, void *data)
        {
            auto *raw = jsc_value_to_string(value);
            std::string message{raw};
            g_free(raw);

            reinterpret_cast<webview *>(data)->on_message(message);
        };

        m_impl->message_received =
            g_signal_connect(m_impl->manager, "script-message-received", G_CALLBACK(+on_message), this);

        auto on_load = [](WebKitWebView *, WebKitLoadEvent event, void *data)
        {
            auto *self = reinterpret_cast<webview *>(data);

            if (event == WEBKIT_LOAD_COMMITTED)
            {
                self->m_events.at<web_event::url_changed>().fire(self->url());
                return;
            }

            if (event == WEBKIT_LOAD_FINISHED)
            {
                self->m_events.at<web_event::load_finished>().fire();
                return;
            }

            if (event != WEBKIT_LOAD_STARTED)
            {
                return;
            }

            self->m_impl->dom_loaded = false;
            self->m_events.at<web_event::load_started>().fire();
        };

        g_signal_connect(m_impl->web_view.get(), "load-changed", G_CALLBACK(+on_load), this);

        auto *controller = gtk_gesture_click_new();

        auto on_click = [](GtkGestureClick *gesture, gint, double, double, void *data)
        {
            auto *self       = reinterpret_cast<webview *>(data);
            auto *controller = GTK_EVENT_CONTROLLER(gesture);
            auto *event      = gtk_event_controller_get_current_event(controller);

            self->window::m_impl->prev_click = {
                .event      = g_event_ptr::copy(event),
                .controller = controller,
            };
        };

        g_signal_connect(controller, "pressed", G_CALLBACK(+on_click), this);
        gtk_widget_add_controller(GTK_WIDGET(m_impl->web_view.get()), GTK_EVENT_CONTROLLER(controller));

        inject(impl::inject_script(), load_time::creation);
        inject(std::string{impl::ready_script}, load_time::ready);
    }

    webview::~webview()
    {
        g_signal_handler_disconnect(m_impl->manager, m_impl->message_received);
    }

    bool webview::on_message(const std::string &message)
    {
        if (message == "dom_loaded")
        {
            m_impl->dom_loaded = true;

            for (const auto &pending : m_impl->pending)
            {
                execute(pending);
            }

            m_impl->pending.clear();
            m_events.at<web_event::dom_ready>().fire();

            return true;
        }

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

        return {{g_object_ptr<GdkTexture>::copy(webkit_web_view_get_favicon(m_impl->web_view.get()))}};
    }

    std::string webview::page_title() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return page_title(); }).get();
        }

        return webkit_web_view_get_title(m_impl->web_view.get());
    }

    bool webview::dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return dev_tools(); }).get();
        }

        auto *settings = webkit_web_view_get_settings(m_impl->web_view.get());
        return webkit_settings_get_enable_developer_extras(settings);
    }

    std::string webview::url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return url(); }).get();
        }

        const auto *rtn = webkit_web_view_get_uri(m_impl->web_view.get());

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

        GdkRGBA color{};
        webkit_web_view_get_background_color(m_impl->web_view.get(), &color);

        return {
            static_cast<std::uint8_t>(color.red * 255.f),
            static_cast<std::uint8_t>(color.green * 255.f),
            static_cast<std::uint8_t>(color.blue * 255.f),
            static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bool webview::force_dark_mode() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this] { return force_dark_mode(); }).get();
        }

        AdwColorScheme scheme{};
        g_object_get(adw_style_manager_get_default(), "color-scheme", &scheme, nullptr);

        return scheme == ADW_COLOR_SCHEME_FORCE_DARK;
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_dev_tools(enabled); }).get();
        }

        auto *settings  = webkit_web_view_get_settings(m_impl->web_view.get());
        auto *inspector = webkit_web_view_get_inspector(m_impl->web_view.get());

        webkit_settings_set_enable_developer_extras(settings, enabled);
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

    void webview::set_force_dark_mode(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_force_dark_mode(enabled); }).get();
        }

        g_object_set(adw_style_manager_get_default(), "color-scheme",
                     enabled ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT, nullptr);
    }

    void webview::set_background(const color &background)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, background] { return set_background(background); }).get();
        }

        auto [r, g, b, a] = background;

        GdkRGBA color{
            .red   = static_cast<float>(r) / 255.f,
            .green = static_cast<float>(g) / 255.f,
            .blue  = static_cast<float>(b) / 255.f,
            .alpha = static_cast<float>(a) / 255.f,
        };

        webkit_web_view_set_background_color(m_impl->web_view.get(), &color);
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

        webkit_web_view_load_uri(m_impl->web_view.get(), url.c_str());
    }

    void webview::serve(const std::string &file, const std::string &scheme)
    {
        set_url(fmt::format("{}:/{}", scheme, file));
    }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this]() { return clear_scripts(); }).get();
        }

        auto *manager = webkit_web_view_get_user_content_manager(m_impl->web_view.get());

        for (const auto &script : m_impl->scripts | std::views::drop(2))
        {
            webkit_user_content_manager_remove_script(manager, script.get());
        }

        if (m_impl->scripts.empty())
        {
            return;
        }

        m_impl->scripts.resize(2);
    }

    void webview::execute(const std::string &code)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, code]() { return execute(code); }).get();
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(code);
            return;
        }

        webkit_web_view_evaluate_javascript(m_impl->web_view.get(), code.c_str(), -1, nullptr, nullptr, nullptr,
                                            nullptr, nullptr);
    }

    void webview::inject(const std::string &code, load_time time, web_frame frame)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, code, time, frame]() { return inject(code, time, frame); }).get();
        }

        auto webkit_time  = time == load_time::creation ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
                                                        : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;
        auto webkit_frame = frame == web_frame::all ? WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES //
                                                    : WEBKIT_USER_CONTENT_INJECT_TOP_FRAME;

        auto *manager = webkit_web_view_get_user_content_manager(m_impl->web_view.get());
        auto *script  = webkit_user_script_new(code.c_str(), webkit_frame, webkit_time, nullptr, nullptr);

        m_impl->scripts.emplace_back(script);
        webkit_user_content_manager_add_script(manager, script);
    }

    void webview::handle_scheme(const std::string &name, scheme_handler handler)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, name, handler = std::move(handler)] mutable
                            { return handle_scheme(name, std::move(handler)); })
                .get();
        }

        if (m_impl->schemes.contains(name))
        {
            return;
        }

        auto *context  = webkit_web_view_get_context(m_impl->web_view.get());
        auto *security = webkit_web_context_get_security_manager(context);

        auto state    = std::make_unique<scheme_state>(std::move(handler));
        auto callback = reinterpret_cast<WebKitURISchemeRequestCallback>(&scheme_state::handle);

        webkit_web_context_register_uri_scheme(context, name.c_str(), callback, state.get(), nullptr);
        m_impl->schemes.emplace(name, std::move(state));

        webkit_security_manager_register_uri_scheme_as_secure(security, name.c_str());
        webkit_security_manager_register_uri_scheme_as_cors_enabled(security, name.c_str());
    }

    void webview::remove_scheme(const std::string &name)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, name] { return remove_scheme(name); }).get();
        }

        if (!m_impl->schemes.contains(name))
        {
            return;
        }

        m_impl->schemes.at(name)->handler = nullptr;
    }

    void webview::clear(web_event event)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, event]() { return clear(event); }).get();
        }

        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, event, id] { return remove(event, id); }).get();
        }

        m_events.remove(event, id);
    }

    template <web_event Event>
    void webview::once(events::type<Event> callback)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable
                            { return once<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type<Event> callback)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return on<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    void webview::register_scheme(const std::string &)
    {
        //? Registering schemes before hand is not required for webkit.
    }

    INSTANTIATE_EVENTS(webview, 6, web_event)
} // namespace saucer
