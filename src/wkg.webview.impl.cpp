#include "wkg.webview.impl.hpp"

#include "gtk.window.impl.hpp"
#include "wkg.scheme.impl.hpp"

#include "wkg.navigation.impl.hpp"
#include "wkg.permission.impl.hpp"

#include <cassert>
#include <optional>

#include <json-glib/json-glib.h>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::permission::type> = true;

namespace saucer
{
    using native = webview::impl::native;
    using event  = webview::event;

    template <auto Func, auto Value>
    static constexpr auto permission_update = [](gpointer raw, permission::type &result) -> gboolean
    {
        const auto rtn = Func(raw);

        if (rtn)
        {
            result |= Value;
        }

        return rtn;
    };

    static constexpr auto translate_media_permission = [](gpointer raw, permission::type &result) -> gboolean
    {
        if (!WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(raw))
        {
            return false;
        }

        auto *const permission = WEBKIT_USER_MEDIA_PERMISSION_REQUEST(raw);

        if (webkit_user_media_permission_is_for_audio_device(permission))
        {
            result |= permission::type::audio_media;
        }

        if (webkit_user_media_permission_is_for_video_device(permission))
        {
            result |= permission::type::video_media;
        }

        if (webkit_user_media_permission_is_for_display_device(permission))
        {
            result |= permission::type::desktop_media;
        }

        return true;
    };

    template <>
    void native::setup<event::permission>(impl *self)
    {
        auto &event = self->events.get<event::permission>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitPermissionRequest *raw, impl *self)
        {
            using permission::request;
            using enum permission::type;

            static constexpr auto mappings = std::array<gboolean (*)(gpointer, permission::type &), 6>{
                permission_update<WEBKIT_IS_CLIPBOARD_PERMISSION_REQUEST, clipboard>,
                permission_update<WEBKIT_IS_DEVICE_INFO_PERMISSION_REQUEST, device_info>,
                permission_update<WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST, location>,
                permission_update<WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST, notification>,
                permission_update<WEBKIT_IS_POINTER_LOCK_PERMISSION_REQUEST, mouse_lock>,
                translate_media_permission,
            };

            auto type = unknown;

            for (const auto &check : mappings)
            {
                if (!check(raw, type))
                {
                    continue;
                }

                break;
            }

            auto req = std::make_shared<request>(request::impl{
                .request = utils::g_object_ptr<WebKitPermissionRequest>::ref(raw),
                .url     = self->url().value_or({}),
                .type    = type,
            });

            self->events.get<event::permission>().fire(req).find(status::handled);
        };

        const auto id = utils::connect(web_view, "permission-request", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void native::setup<event::fullscreen>(impl *self)
    {
        auto &event = self->events.get<event::fullscreen>();

        if (!event.empty())
        {
            return;
        }

        auto enter_callback = [](WebKitWebView *, impl *self) -> gboolean
        {
            return self->events.get<event::fullscreen>().fire(true).find(policy::block).has_value();
        };

        auto leave_callback = [](WebKitWebView *, impl *self) -> gboolean
        {
            return self->events.get<event::fullscreen>().fire(false).find(policy::block).has_value();
        };

        const auto enter = utils::connect(web_view, "enter-fullscreen", +enter_callback, self);
        const auto leave = utils::connect(web_view, "leave-fullscreen", +leave_callback, self);

        event.on_clear(
            [this, enter, leave]
            {
                g_signal_handler_disconnect(web_view, enter);
                g_signal_handler_disconnect(web_view, leave);
            });
    }

    template <>
    void native::setup<event::dom_ready>(impl *)
    {
    }

    template <>
    void native::setup<event::navigated>(impl *)
    {
    }

    template <>
    void native::setup<event::navigate>(impl *self)
    {
        auto &event = self->events.get<event::navigate>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitPolicyDecision *raw, WebKitPolicyDecisionType type, impl *self) -> gboolean
        {
            if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
            {
                return false;
            }

            auto *const decision = WEBKIT_NAVIGATION_POLICY_DECISION(raw);

            auto nav = navigation{navigation::impl{
                .decision = decision,
                .type     = type,
            }};

            if (self->events.get<event::navigate>().fire(nav).find(policy::block))
            {
                webkit_policy_decision_ignore(raw);
                return true;
            }

            return false;
        };

        const auto id = utils::connect(web_view, "decide-policy", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void native::setup<event::message>(impl *)
    {
    }

    template <>
    void native::setup<event::request>(impl *self)
    {
        auto &event = self->events.get<event::request>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitWebResource *, WebKitURIRequest *request, impl *self)
        {
            const auto *raw = webkit_uri_request_get_uri(request);

            if (!raw)
            {
                return;
            }

            auto url = uri::parse(raw);

            if (!url.has_value())
            {
                assert(false);
                return;
            }

            self->events.get<event::request>().fire(url.value());
        };

        const auto id = utils::connect(web_view, "resource-load-started", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void native::setup<event::favicon>(impl *self)
    {
        auto &event = self->events.get<event::favicon>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, impl *self)
        {
            self->events.get<event::favicon>().fire(self->favicon());
        };

        const auto id = utils::connect(web_view, "notify::favicon", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void native::setup<event::title>(impl *self)
    {
        auto &event = self->events.get<event::title>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, impl *self)
        {
            self->events.get<event::title>().fire(self->page_title());
        };

        const auto id = utils::connect(web_view, "notify::title", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void native::setup<event::load>(impl *)
    {
    }

    gboolean native::on_context(WebKitWebView *, WebKitContextMenu *, WebKitHitTestResult *, impl *self)
    {
        return !self->platform->context_menu;
    }

    void native::on_message(WebKitWebView *, JSCValue *value, impl *self)
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
            self->events.get<event::dom_ready>().fire();

            return;
        }

        self->events.get<event::message>().fire(message).find(status::handled);
    }

    void native::on_load(WebKitWebView *, WebKitLoadEvent event, impl *self)
    {
        auto url = self->url();

        if (!url.has_value())
        {
            assert(false);
            return;
        }

        if (event == WEBKIT_LOAD_COMMITTED)
        {
            self->events.get<event::navigated>().fire(url.value());
            return;
        }

        if (event == WEBKIT_LOAD_FINISHED)
        {
            self->events.get<event::load>().fire(state::finished);
            return;
        }

        if (event != WEBKIT_LOAD_STARTED)
        {
            return;
        }

        self->platform->dom_loaded = false;
        self->events.get<event::load>().fire(state::started);
    }

    void native::on_click(GtkGestureClick *gesture, gint, gdouble, gdouble, impl *self)
    {
        auto *const controller = GTK_EVENT_CONTROLLER(gesture);
        auto *const event      = gtk_event_controller_get_current_event(controller);

        self->window->native<false>()->platform->prev_click.emplace(click_event{
            .event      = utils::g_event_ptr::ref(event),
            .controller = controller,
        });
    }

    void native::on_release(GtkGestureClick *, gdouble, gdouble, guint, GdkEventSequence *, impl *self)
    {
        auto &previous = self->window->native<false>()->platform->prev_resizable;

        if (!previous.has_value())
        {
            return;
        }

        self->window->set_resizable(previous.value());
        previous.reset();
    }

    WebKitSettings *native::make_settings(const options &opts)
    {
        std::vector<GValue> values;
        std::vector<std::string> names;

        auto parser = utils::g_object_ptr<JsonParser>(json_parser_new());

        for (const auto &flag : opts.browser_flags)
        {
            const auto delim = flag.find('=');

            if (delim == std::string::npos)
            {
                continue;
            }

            auto key   = flag.substr(0, delim);
            auto value = flag.substr(delim + 1);

            const auto data = std::format(R"json({{ "value": {} }})json", value);

            if (!json_parser_load_from_data(parser.get(), data.c_str(), static_cast<gssize>(data.size()), nullptr))
            {
                continue;
            }

            auto *const root   = json_parser_get_root(parser.get());
            auto *const object = json_node_get_object(root);
            auto *const node   = json_object_get_member(object, "value");

            GValue parsed = G_VALUE_INIT;
            json_node_get_value(node, &parsed);

            values.emplace_back(parsed);
            names.emplace_back(std::move(key));
        }

        auto raw_names = std::views::transform(names, &std::string::c_str) //
                         | std::ranges::to<std::vector<const char *>>();

        // https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/glib/WebKitSettings.cpp#L1753
        auto *const settings = g_object_new_with_properties(WEBKIT_TYPE_SETTINGS, values.size(), raw_names.data(), values.data());

        for (auto &value : values)
        {
            g_value_unset(&value);
        }

        return WEBKIT_SETTINGS(settings);
    }
} // namespace saucer
