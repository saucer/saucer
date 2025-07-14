#include "wkg.webview.impl.hpp"

#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"
#include "wkg.scheme.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

namespace saucer
{
    using impl = webview::impl;

    impl::impl() = default;

    bool impl::init_platform(const options &opts)
    {
        platform = std::make_unique<native>();

        static std::once_flag flag;
        std::call_once(flag, [] { register_scheme("saucer"); });

        platform->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
        platform->settings = native::make_settings(opts);

        auto *const session      = webkit_web_view_get_network_session(platform->web_view);
        auto *const data_manager = webkit_network_session_get_website_data_manager(session);

        webkit_website_data_manager_set_favicons_enabled(data_manager, true);

        if (!opts.user_agent.empty())
        {
            webkit_settings_set_user_agent(platform->settings.get(), opts.user_agent.c_str());
        }

        if (opts.persistent_cookies)
        {
            auto *const manager = webkit_network_session_get_cookie_manager(session);
            auto path           = opts.storage_path;

            if (path.empty())
            {
                path = fs::current_path() / ".saucer";
            }

            webkit_cookie_manager_set_persistent_storage(manager, path.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
        }

        webkit_settings_set_hardware_acceleration_policy(platform->settings.get(), opts.hardware_acceleration
                                                                                       ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
                                                                                       : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

        webkit_web_view_set_settings(platform->web_view, platform->settings.get());

        gtk_widget_set_size_request(GTK_WIDGET(platform->web_view), 1, 1);
        gtk_widget_set_vexpand(GTK_WIDGET(platform->web_view), true);
        gtk_widget_set_hexpand(GTK_WIDGET(platform->web_view), true);

        gtk_box_append(window->native<false>()->platform->content, GTK_WIDGET(platform->web_view));

        auto on_context = [](WebKitWebView *, WebKitContextMenu *, WebKitHitTestResult *, impl *data) -> gboolean
        {
            return !data->platform->context_menu;
        };
        utils::connect(platform->web_view, "context-menu", +on_context, this);

        platform->manager = webkit_web_view_get_user_content_manager(platform->web_view);
        webkit_user_content_manager_register_script_message_handler(platform->manager, "saucer", nullptr);

        auto on_message = [](WebKitWebView *, JSCValue *value, impl *self)
        {
            auto message = std::string{utils::g_str_ptr{jsc_value_to_string(value)}.get()};

            if (message == "dom_loaded")
            {
                self->platform->dom_loaded = true;

                for (const auto &pending : self->platform->pending)
                {
                    self->execute(pending);
                }

                self->platform->pending.clear();
                self->events->get<event::dom_ready>().fire();

                return;
            }

            self->events->get<event::message>().fire(message).find(status::handled);
        };

        platform->msg_received = utils::connect(platform->manager, "script-message-received", +on_message, this);

        auto on_load = [](WebKitWebView *, WebKitLoadEvent event, impl *self)
        {
            auto url = self->url();

            if (!url.has_value())
            {
                assert(false);
                return;
            }

            if (event == WEBKIT_LOAD_COMMITTED)
            {
                self->events->get<event::navigated>().fire(url.value());
                return;
            }

            if (event == WEBKIT_LOAD_FINISHED)
            {
                self->events->get<event::load>().fire(state::finished);
                return;
            }

            if (event != WEBKIT_LOAD_STARTED)
            {
                return;
            }

            self->platform->dom_loaded = false;
            self->events->get<event::load>().fire(state::started);
        };

        utils::connect(platform->web_view, "load-changed", +on_load, this);

        auto *const controller = gtk_gesture_click_new();

        auto on_click = [](GtkGestureClick *gesture, gint, gdouble, gdouble, impl *self)
        {
            auto *const controller = GTK_EVENT_CONTROLLER(gesture);
            auto *const event      = gtk_event_controller_get_current_event(controller);

            self->window->native<false>()->platform->prev_click.emplace(click_event{
                .event      = utils::g_event_ptr::ref(event),
                .controller = controller,
            });
        };

        auto release = [](GtkGestureClick *, gdouble, gdouble, guint, GdkEventSequence *, impl *self)
        {
            auto &previous = self->window->native<false>()->platform->prev_resizable;

            if (!previous.has_value())
            {
                return;
            }

            self->window->set_resizable(previous.value());
            previous.reset();
        };

        utils::connect(controller, "pressed", +on_click, this);
        utils::connect(controller, "unpaired-release", +release, this);

        gtk_widget_add_controller(GTK_WIDGET(platform->web_view), GTK_EVENT_CONTROLLER(controller));

        inject({.code = native::inject_script(), .time = load_time::creation, .permanent = true});
        inject({.code = std::string{native::ready_script}, .time = load_time::ready, .permanent = true});

        return true;
    }

    impl::~impl()
    {
        for (const auto &[name, _] : native::schemes)
        {
            remove_scheme(name);
        }

        gtk_box_remove(window->native<false>()->platform->content, GTK_WIDGET(platform->web_view));
        g_signal_handler_disconnect(platform->manager, platform->msg_received);
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    icon impl::favicon() const
    {
        return {{utils::g_object_ptr<GdkTexture>::ref(webkit_web_view_get_favicon(platform->web_view))}};
    }

    std::string impl::page_title() const
    {
        const auto *rtn = webkit_web_view_get_title(platform->web_view);

        if (!rtn)
        {
            return {};
        }

        return rtn;
    }

    bool impl::dev_tools() const
    {
        auto *const settings = webkit_web_view_get_settings(platform->web_view);
        return webkit_settings_get_enable_developer_extras(settings);
    }

    bool impl::context_menu() const
    {
        return platform->context_menu;
    }

    std::optional<uri> impl::url() const
    {
        const auto *url = webkit_web_view_get_uri(platform->web_view);

        if (!url)
        {
            return std::nullopt;
        }

        return uri::parse(url).value_or({});
    }

    color impl::background() const
    {
        GdkRGBA color{};
        webkit_web_view_get_background_color(platform->web_view, &color);

        return {
            static_cast<std::uint8_t>(color.red * 255.f),
            static_cast<std::uint8_t>(color.green * 255.f),
            static_cast<std::uint8_t>(color.blue * 255.f),
            static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bool impl::force_dark_mode() const // NOLINT(*-static)
    {
        AdwColorScheme scheme{};
        g_object_get(adw_style_manager_get_default(), "color-scheme", &scheme, nullptr);

        return scheme == ADW_COLOR_SCHEME_FORCE_DARK;
    }

    void impl::set_dev_tools(bool enabled) // NOLINT(*-function-const)
    {
        auto *const settings  = webkit_web_view_get_settings(platform->web_view);
        auto *const inspector = webkit_web_view_get_inspector(platform->web_view);

        webkit_settings_set_enable_developer_extras(settings, enabled);

        if (!enabled)
        {
            webkit_web_inspector_close(inspector);
            return;
        }

        webkit_web_inspector_show(inspector);
    }

    void impl::set_context_menu(bool enabled) // NOLINT(*-function-const)
    {
        platform->context_menu = enabled;
    }

    void impl::set_force_dark_mode(bool enabled) // NOLINT(*-static, *-function-const)
    {
        g_object_set(adw_style_manager_get_default(), "color-scheme", enabled ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT,
                     nullptr);
    }

    void impl::set_background(const color &color) // NOLINT(*-function-const)
    {
        const auto [r, g, b, a] = color;

        GdkRGBA rgba{
            .red   = static_cast<float>(r) / 255.f,
            .green = static_cast<float>(g) / 255.f,
            .blue  = static_cast<float>(b) / 255.f,
            .alpha = static_cast<float>(a) / 255.f,
        };

        webkit_web_view_set_background_color(platform->web_view, &rgba);
        window->native<false>()->platform->make_transparent(a < 255);
    }

    void impl::set_url(const uri &url) // NOLINT(*-function-const)
    {
        webkit_web_view_load_uri(platform->web_view, url.string().c_str());
    }

    void impl::back() // NOLINT(*-function-const)
    {
        webkit_web_view_go_back(platform->web_view);
    }

    void impl::forward() // NOLINT(*-function-const)
    {
        webkit_web_view_go_forward(platform->web_view);
    }

    void impl::reload() // NOLINT(*-function-const)
    {
        webkit_web_view_reload(platform->web_view);
    }

    void impl::clear_scripts() // NOLINT(*-function-const)
    {
        auto *const manager = webkit_web_view_get_user_content_manager(platform->web_view);

        for (auto it = platform->scripts.begin(); it != platform->scripts.end();)
        {
            const auto &[script, permanent] = *it;

            if (permanent)
            {
                ++it;
                continue;
            }

            webkit_user_content_manager_remove_script(manager, script.get());
            it = platform->scripts.erase(it);
        }
    }

    void impl::inject(const script &script) // NOLINT(*-function-const)
    {
        const auto time = script.time == load_time::creation ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
                                                             : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;

        const auto frame = script.frame == web_frame::all ? WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES //
                                                          : WEBKIT_USER_CONTENT_INJECT_TOP_FRAME;

        auto *const manager     = webkit_web_view_get_user_content_manager(platform->web_view);
        auto *const user_script = webkit_user_script_new(script.code.c_str(), frame, time, nullptr, nullptr);

        platform->scripts.emplace_back(user_script, script.permanent);
        webkit_user_content_manager_add_script(manager, user_script);
    }

    void impl::execute(const std::string &code) // NOLINT(*-function-const)
    {
        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        webkit_web_view_evaluate_javascript(platform->web_view, code.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    void impl::handle_scheme(const std::string &name, scheme::resolver &&resolver) // NOLINT(*-function-const)
    {
        if (!native::schemes.contains(name))
        {
            return;
        }

        // TODO: Use heterogenous lookup with C++26
        native::schemes[name]->add_callback(platform->web_view, std::move(resolver));
    }

    void impl::remove_scheme(const std::string &name) // NOLINT(*-function-const)
    {
        if (!native::schemes.contains(name))
        {
            return;
        }

        native::schemes[name]->del_callback(platform->web_view);
    }

    void impl::register_scheme(const std::string &name)
    {
        if (native::schemes.contains(name))
        {
            return;
        }

        auto *const context  = webkit_web_context_get_default();
        auto *const security = webkit_web_context_get_security_manager(context);

        auto handler  = std::make_unique<scheme::handler>();
        auto callback = reinterpret_cast<WebKitURISchemeRequestCallback>(&scheme::handler::handle);

        webkit_web_context_register_uri_scheme(context, name.c_str(), callback, handler.get(), nullptr);
        native::schemes.emplace(name, std::move(handler));

        webkit_security_manager_register_uri_scheme_as_secure(security, name.c_str());
        webkit_security_manager_register_uri_scheme_as_cors_enabled(security, name.c_str());
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT);
} // namespace saucer
