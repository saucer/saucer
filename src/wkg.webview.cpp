#include "wkg.webview.impl.hpp"

#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"
#include "wkg.scheme.impl.hpp"

#include "handle.hpp"
#include "instantiate.hpp"

#include <format>

namespace saucer
{
    webview::webview(const preferences &prefs) : window(prefs), extensible(this), m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;
        std::call_once(flag, [] { register_scheme("saucer"); });

        m_impl->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
        m_impl->settings = impl::make_settings(prefs);

        auto *const session      = webkit_web_view_get_network_session(m_impl->web_view);
        auto *const data_manager = webkit_network_session_get_website_data_manager(session);

        webkit_website_data_manager_set_favicons_enabled(data_manager, true);

        if (!prefs.user_agent.empty())
        {
            webkit_settings_set_user_agent(m_impl->settings.get(), prefs.user_agent.c_str());
        }

        if (prefs.persistent_cookies)
        {
            auto *const manager = webkit_network_session_get_cookie_manager(session);
            auto path           = prefs.storage_path;

            if (path.empty())
            {
                path = fs::current_path() / ".saucer";
            }

            webkit_cookie_manager_set_persistent_storage(manager, path.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
        }

        webkit_settings_set_hardware_acceleration_policy(m_impl->settings.get(), prefs.hardware_acceleration
                                                                                     ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
                                                                                     : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

        webkit_web_view_set_settings(m_impl->web_view, m_impl->settings.get());

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->web_view), 1, 1);
        gtk_widget_set_vexpand(GTK_WIDGET(m_impl->web_view), true);
        gtk_widget_set_hexpand(GTK_WIDGET(m_impl->web_view), true);

        gtk_box_append(window::m_impl->content, GTK_WIDGET(m_impl->web_view));

        auto on_context = [](WebKitWebView *, WebKitContextMenu *, WebKitHitTestResult *, impl *data)
        {
            return static_cast<gboolean>(!data->context_menu);
        };
        g_signal_connect(m_impl->web_view, "context-menu", G_CALLBACK(+on_context), m_impl.get());

        m_impl->manager = webkit_web_view_get_user_content_manager(m_impl->web_view);
        webkit_user_content_manager_register_script_message_handler(m_impl->manager, "saucer", nullptr);

        auto on_message = [](WebKitWebView *, JSCValue *value, void *data)
        {
            auto message = std::string{utils::handle<char *, g_free>{jsc_value_to_string(value)}.get()};
            auto &self   = *reinterpret_cast<webview *>(data);

            if (message == "dom_loaded")
            {
                self.m_impl->dom_loaded = true;

                for (const auto &pending : self.m_impl->pending)
                {
                    self.execute(pending);
                }

                self.m_impl->pending.clear();
                self.m_events.at<web_event::dom_ready>().fire();

                return;
            }

            self.on_message(message);
        };

        m_impl->msg_received = g_signal_connect(m_impl->manager, "script-message-received", G_CALLBACK(+on_message), this);

        auto on_load = [](WebKitWebView *, WebKitLoadEvent event, void *data)
        {
            auto *const self = reinterpret_cast<webview *>(data);

            if (event == WEBKIT_LOAD_COMMITTED)
            {
                self->m_events.at<web_event::navigated>().fire(self->url());
                return;
            }

            if (event == WEBKIT_LOAD_FINISHED)
            {
                self->m_events.at<web_event::load>().fire(state::finished);
                return;
            }

            if (event != WEBKIT_LOAD_STARTED)
            {
                return;
            }

            self->m_impl->dom_loaded = false;
            self->m_events.at<web_event::load>().fire(state::started);
        };

        g_signal_connect(m_impl->web_view, "load-changed", G_CALLBACK(+on_load), this);

        auto *const controller = gtk_gesture_click_new();

        auto on_click = [](GtkGestureClick *gesture, gint, gdouble, gdouble, void *data)
        {
            auto *const self       = reinterpret_cast<webview *>(data);
            auto *const controller = GTK_EVENT_CONTROLLER(gesture);
            auto *const event      = gtk_event_controller_get_current_event(controller);

            self->window::m_impl->prev_click.emplace(click_event{
                .event      = utils::g_event_ptr::ref(event),
                .controller = controller,
            });
        };

        auto release = [](GtkGestureClick *, gdouble, gdouble, guint, GdkEventSequence *, void *data)
        {
            auto *const self = reinterpret_cast<webview *>(data);
            auto &previous   = self->window::m_impl->prev_resizable;

            if (!previous.has_value())
            {
                return;
            }

            self->set_resizable(previous.value());
            previous.reset();
        };

        g_signal_connect(controller, "pressed", G_CALLBACK(+on_click), this);
        g_signal_connect(controller, "unpaired-release", G_CALLBACK(+release), this);

        gtk_widget_add_controller(GTK_WIDGET(m_impl->web_view), GTK_EVENT_CONTROLLER(controller));

        inject({.code = impl::inject_script(), .time = load_time::creation, .permanent = true});
        inject({.code = std::string{impl::ready_script}, .time = load_time::ready, .permanent = true});
    }

    webview::~webview()
    {
        for (const auto &[name, _] : impl::schemes)
        {
            remove_scheme(name);
        }

        gtk_box_remove(window::m_impl->content, GTK_WIDGET(m_impl->web_view));
        g_signal_handler_disconnect(m_impl->manager, m_impl->msg_received);
    }

    icon webview::favicon() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return favicon(); });
        }

        return {{utils::g_object_ptr<GdkTexture>::ref(webkit_web_view_get_favicon(m_impl->web_view))}};
    }

    std::string webview::page_title() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return page_title(); });
        }

        return webkit_web_view_get_title(m_impl->web_view);
    }

    bool webview::dev_tools() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return dev_tools(); });
        }

        auto *const settings = webkit_web_view_get_settings(m_impl->web_view);
        return webkit_settings_get_enable_developer_extras(settings);
    }

    std::string webview::url() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return url(); });
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
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return context_menu(); });
        }

        return m_impl->context_menu;
    }

    color webview::background() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return background(); });
        }

        GdkRGBA color{};
        webkit_web_view_get_background_color(m_impl->web_view, &color);

        return {
            static_cast<std::uint8_t>(color.red * 255.f),
            static_cast<std::uint8_t>(color.green * 255.f),
            static_cast<std::uint8_t>(color.blue * 255.f),
            static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bool webview::force_dark_mode() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return force_dark_mode(); });
        }

        AdwColorScheme scheme{};
        g_object_get(adw_style_manager_get_default(), "color-scheme", &scheme, nullptr);

        return scheme == ADW_COLOR_SCHEME_FORCE_DARK;
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_dev_tools(enabled); });
        }

        auto *const settings  = webkit_web_view_get_settings(m_impl->web_view);
        auto *const inspector = webkit_web_view_get_inspector(m_impl->web_view);

        webkit_settings_set_enable_developer_extras(settings, enabled);

        if (!enabled)
        {
            webkit_web_inspector_close(inspector);
            return;
        }

        webkit_web_inspector_show(inspector);
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_context_menu(enabled); });
        }

        m_impl->context_menu = enabled;
    }

    void webview::set_force_dark_mode(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_force_dark_mode(enabled); });
        }

        g_object_set(adw_style_manager_get_default(), "color-scheme", enabled ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT,
                     nullptr);
    }

    void webview::set_background(const color &color)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, color] { return set_background(color); });
        }

        const auto [r, g, b, a] = color;

        GdkRGBA rgba{
            .red   = static_cast<float>(r) / 255.f,
            .green = static_cast<float>(g) / 255.f,
            .blue  = static_cast<float>(b) / 255.f,
            .alpha = static_cast<float>(a) / 255.f,
        };

        webkit_web_view_set_background_color(m_impl->web_view, &rgba);
        window::m_impl->make_transparent(a < 255);
    }

    void webview::set_file(const fs::path &file)
    {
        auto error     = std::error_code{};
        auto canonical = fs::canonical(file, error);

        if (error)
        {
            return;
        }

        set_url(std::format("file://{}", canonical.string()));
    }

    void webview::set_url(const std::string &url)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, url] { return set_url(url); });
        }

        webkit_web_view_load_uri(m_impl->web_view, url.c_str());
    }

    void webview::back()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return back(); });
        }

        webkit_web_view_go_back(m_impl->web_view);
    }

    void webview::forward()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return forward(); });
        }

        webkit_web_view_go_forward(m_impl->web_view);
    }

    void webview::reload()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return reload(); });
        }

        webkit_web_view_reload(m_impl->web_view);
    }

    void webview::clear_scripts()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return clear_scripts(); });
        }

        auto *const manager = webkit_web_view_get_user_content_manager(m_impl->web_view);

        for (auto it = m_impl->scripts.begin(); it != m_impl->scripts.end();)
        {
            const auto &[script, permanent] = *it;

            if (permanent)
            {
                ++it;
                continue;
            }

            webkit_user_content_manager_remove_script(manager, script.get());
            it = m_impl->scripts.erase(it);
        }
    }

    void webview::inject(const script &script)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, script] { return inject(script); });
        }

        const auto time  = script.time == load_time::creation ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
                                                              : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;
        const auto frame = script.frame == web_frame::all ? WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES //
                                                          : WEBKIT_USER_CONTENT_INJECT_TOP_FRAME;

        auto *const manager     = webkit_web_view_get_user_content_manager(m_impl->web_view);
        auto *const user_script = webkit_user_script_new(script.code.c_str(), frame, time, nullptr, nullptr);

        m_impl->scripts.emplace_back(user_script, script.permanent);
        webkit_user_content_manager_add_script(manager, user_script);
    }

    void webview::execute(const std::string &code)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, code] { return execute(code); });
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(code);
            return;
        }

        webkit_web_view_evaluate_javascript(m_impl->web_view, code.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    void webview::handle_scheme(const std::string &name, scheme::resolver &&resolver)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, name, handler = std::move(resolver)] mutable
                                      { return handle_scheme(name, std::move(handler)); });
        }

        if (!impl::schemes.contains(name))
        {
            return;
        }

        // TODO: Use heterogenous lookup with C++26

        impl::schemes[name]->add_callback(m_impl->web_view, {.app = m_parent, .resolver = std::move(resolver)});
    }

    void webview::remove_scheme(const std::string &name)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, name] { return remove_scheme(name); });
        }

        if (!impl::schemes.contains(name))
        {
            return;
        }

        impl::schemes[name]->del_callback(m_impl->web_view);
    }

    void webview::clear(web_event event)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event] { return clear(event); });
        }

        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event, id] { return remove(event, id); });
        }

        m_events.remove(event, id);
    }

    template <web_event Event>
    void webview::once(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, callback = std::move(callback)] mutable { return once<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, callback = std::move(callback)] mutable { return on<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    void webview::register_scheme(const std::string &name)
    {
        if (impl::schemes.contains(name))
        {
            return;
        }

        auto *const context  = webkit_web_context_get_default();
        auto *const security = webkit_web_context_get_security_manager(context);

        auto handler  = std::make_unique<scheme::handler>();
        auto callback = reinterpret_cast<WebKitURISchemeRequestCallback>(&scheme::handler::handle);

        webkit_web_context_register_uri_scheme(context, name.c_str(), callback, handler.get(), nullptr);
        impl::schemes.emplace(name, std::move(handler));

        webkit_security_manager_register_uri_scheme_as_secure(security, name.c_str());
        webkit_security_manager_register_uri_scheme_as_cors_enabled(security, name.c_str());
    }

    SAUCER_INSTANTIATE_EVENTS(6, webview, web_event);
} // namespace saucer
