#include "wkg.webview.impl.hpp"

#include "error.impl.hpp" // IWYU pragma: keep

#include "scripts.hpp"
#include "instantiate.hpp"

#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"
#include "wkg.scheme.impl.hpp"

#include <cassert>

namespace saucer
{
    using impl = webview::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        platform = std::make_unique<native>();

        platform->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
        platform->settings = native::make_settings(opts);

        const auto acceleration = opts.hardware_acceleration ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS //
                                                             : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER;

        webkit_settings_set_hardware_acceleration_policy(platform->settings.get(), acceleration);

        if (opts.user_agent.has_value())
        {
            webkit_settings_set_user_agent(platform->settings.get(), opts.user_agent->c_str());
        }

        webkit_web_view_set_settings(platform->web_view, platform->settings.get());

        auto *const session      = webkit_web_view_get_network_session(platform->web_view);
        auto *const data_manager = webkit_network_session_get_website_data_manager(session);

        webkit_website_data_manager_set_favicons_enabled(data_manager, true);

        if (opts.persistent_cookies)
        {
            auto *const manager = webkit_network_session_get_cookie_manager(session);
            auto path           = opts.storage_path.value_or(fs::current_path() / ".saucer");
            webkit_cookie_manager_set_persistent_storage(manager, path.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
        }

        platform->id_context = utils::connect(platform->web_view, "context-menu", native::on_context, this);
        platform->id_load    = utils::connect(platform->web_view, "load-changed", native::on_load, this);

        // The ContentManager is ref'd to prevent it from being destroyed early when using multiple webviews
        platform->manager = content_manager_ptr::ref(webkit_web_view_get_user_content_manager(platform->web_view));
        webkit_user_content_manager_register_script_message_handler(platform->manager.get(), "saucer", nullptr);

        platform->id_message = utils::connect(platform->manager.get(), "script-message-received", native::on_message, this);

        platform->gesture = gtk_gesture_click_new();

        platform->id_click   = utils::connect(platform->gesture, "pressed", native::on_click, this);
        platform->id_release = utils::connect(platform->gesture, "unpaired-release", native::on_release, this);

        gtk_widget_add_controller(GTK_WIDGET(platform->web_view), GTK_EVENT_CONTROLLER(platform->gesture));

        gtk_widget_set_size_request(GTK_WIDGET(platform->web_view), 1, 1);
        gtk_widget_set_vexpand(GTK_WIDGET(platform->web_view), true);
        gtk_widget_set_hexpand(GTK_WIDGET(platform->web_view), true);

        window->native<false>()->platform->add_widget(GTK_WIDGET(platform->web_view));

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        for (const auto &[name, _] : native::schemes)
        {
            remove_scheme(name);
        }

        g_signal_handler_disconnect(platform->web_view, platform->id_context);
        g_signal_handler_disconnect(platform->web_view, platform->id_load);

        g_signal_handler_disconnect(platform->manager.get(), platform->id_message);

        g_signal_handler_disconnect(platform->gesture, platform->id_click);
        g_signal_handler_disconnect(platform->gesture, platform->id_release);

        window->native<false>()->platform->remove_widget(GTK_WIDGET(platform->web_view));
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    result<url> impl::url() const
    {
        const auto *uri = webkit_web_view_get_uri(platform->web_view);

        if (!uri)
        {
            return err(std::errc::not_connected);
        }

        return url::parse(uri);
    }

    icon impl::favicon() const
    {
        return icon::impl{utils::g_object_ptr<GdkTexture>::ref(webkit_web_view_get_favicon(platform->web_view))};
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

    bool impl::force_dark() const // NOLINT(*-static)
    {
        AdwColorScheme scheme{};
        g_object_get(adw_style_manager_get_default(), "color-scheme", &scheme, nullptr);

        return scheme == ADW_COLOR_SCHEME_FORCE_DARK;
    }

    color impl::background() const
    {
        GdkRGBA color{};
        webkit_web_view_get_background_color(platform->web_view, &color);

        return {
            .r = static_cast<std::uint8_t>(color.red * 255.f),
            .g = static_cast<std::uint8_t>(color.green * 255.f),
            .b = static_cast<std::uint8_t>(color.blue * 255.f),
            .a = static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bounds impl::bounds() const
    {
        auto [width, height] = window->size();
        auto *const widget   = GTK_WIDGET(platform->web_view);

        const auto x = gtk_widget_get_margin_start(widget);
        const auto y = gtk_widget_get_margin_top(widget);

        return {
            .x = x,
            .y = y,
            .w = width - x - gtk_widget_get_margin_end(widget),
            .h = height - y - gtk_widget_get_margin_bottom(widget),
        };
    }

    void impl::set_url(const saucer::url &url) // NOLINT(*-function-const)
    {
        webkit_web_view_load_uri(platform->web_view, url.string().c_str());
    }

    void impl::set_html(const std::string &html) // NOLINT(*-function-const)
    {
        webkit_web_view_load_html(platform->web_view, html.c_str(), nullptr);
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

    void impl::set_force_dark(bool enabled) // NOLINT(*-static, *-function-const)
    {
        const auto scheme = enabled ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT;
        g_object_set(adw_style_manager_get_default(), "color-scheme", scheme, nullptr);
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto [r, g, b, a] = color;

        GdkRGBA rgba{
            .red   = static_cast<float>(r) / 255.f,
            .green = static_cast<float>(g) / 255.f,
            .blue  = static_cast<float>(b) / 255.f,
            .alpha = static_cast<float>(a) / 255.f,
        };

        webkit_web_view_set_background_color(platform->web_view, &rgba);
    }

    void impl::reset_bounds() // NOLINT(*-function-const)
    {
        auto *const widget = GTK_WIDGET(platform->web_view);

        gtk_widget_set_margin_start(widget, 0);
        gtk_widget_set_margin_end(widget, 0);
        gtk_widget_set_margin_top(widget, 0);
        gtk_widget_set_margin_bottom(widget, 0);
    }

    void impl::set_bounds(saucer::bounds bounds) // NOLINT(*-function-const)
    {
        auto [width, height] = window->size();
        auto *const widget   = GTK_WIDGET(platform->web_view);

        gtk_widget_set_margin_start(widget, bounds.x);
        gtk_widget_set_margin_end(widget, width - bounds.x - bounds.w);

        gtk_widget_set_margin_top(widget, bounds.y);
        gtk_widget_set_margin_bottom(widget, height - bounds.y - bounds.h);
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

    void impl::execute(const std::string &code) // NOLINT(*-function-const)
    {
        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        webkit_web_view_evaluate_javascript(platform->web_view, code.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    std::size_t impl::inject(const script &script) // NOLINT(*-function-const)
    {
        using enum script::time;

        const auto time = (script.run_at == creation) ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START //
                                                      : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;

        const auto frame = (script.no_frames) ? WEBKIT_USER_CONTENT_INJECT_TOP_FRAME //
                                              : WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES;

        auto *const user_script = webkit_user_script_new(script.code.c_str(), frame, time, nullptr, nullptr);
        const auto id           = platform->id_counter++;

        webkit_user_content_manager_add_script(platform->manager.get(), user_script);
        platform->scripts.emplace(id, wkg_script{.ref = user_script, .clearable = script.clearable});

        return id;
    }

    void impl::uninject() // NOLINT(*-function-const)
    {
        for (auto it = platform->scripts.begin(); it != platform->scripts.end();)
        {
            const auto &[id, script] = *it;

            if (!script.clearable)
            {
                ++it;
                continue;
            }

            webkit_user_content_manager_remove_script(platform->manager.get(), script.ref.get());
            it = platform->scripts.erase(it);
        }
    }

    void impl::uninject(std::size_t id) // NOLINT(*-function-const)
    {
        if (!platform->scripts.contains(id))
        {
            return;
        }

        webkit_user_content_manager_remove_script(platform->manager.get(), platform->scripts[id].ref.get());
        platform->scripts.erase(id);
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

    std::string impl::ready_script()
    {
        return "window.saucer.internal.message('dom_loaded')";
    }

    std::string impl::creation_script()
    {
        static const auto script = std::format(scripts::ipc_script, R"js(
            message: async (message) =>
            {
                window.webkit.messageHandlers.saucer.postMessage(message);
            }
        )js");

        return script;
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT);
} // namespace saucer
