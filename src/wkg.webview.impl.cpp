#include "wkg.webview.impl.hpp"

#include "permission.hpp"
#include "scripts.hpp"
#include "request.hpp"

#include "wkg.scheme.impl.hpp"
#include "wkg.navigation.impl.hpp"
#include "wkg.permission.impl.hpp"

#include <regex>
#include <cassert>

#include <optional>
#include <charconv>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::permission::type> = true;

namespace saucer
{
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
    void saucer::webview::impl::setup<web_event::permission>(webview *self)
    {
        auto &event = self->m_events.get<web_event::permission>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitPermissionRequest *raw, webview *self)
        {
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

            auto request = permission::request{{
                .request = utils::g_object_ptr<WebKitPermissionRequest>::ref(raw),
                .url     = self->url().value_or({}),
                .type    = type,
            }};

            self->m_events.get<web_event::permission>().fire(request);
        };

        const auto id = g_signal_connect(web_view, "permission-request", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void saucer::webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::navigated>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::navigate>(webview *self)
    {
        auto &event = self->m_events.get<web_event::navigate>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, webview *self) -> gboolean
        {
            if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
            {
                return false;
            }

            auto *converted = reinterpret_cast<WebKitNavigationPolicyDecision *>(decision);
            auto nav        = utils::g_object_ptr<WebKitNavigationPolicyDecision>::ref(converted);

            auto request = navigation{{
                .decision = std::move(nav),
                .type     = type,
            }};

            if (self->m_events.get<web_event::navigate>().fire(request).find(policy::block))
            {
                webkit_policy_decision_ignore(decision);
                return true;
            }

            return false;
        };

        const auto id = g_signal_connect(web_view, "decide-policy", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void saucer::webview::impl::setup<web_event::request>(webview *self)
    {
        auto &event = self->m_events.get<web_event::request>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](WebKitWebView *, WebKitWebResource *, WebKitURIRequest *request, webview *self)
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

            self->m_events.get<web_event::request>().fire(url.value());
        };

        const auto id = g_signal_connect(web_view, "resource-load-started", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void saucer::webview::impl::setup<web_event::favicon>(webview *self)
    {
        auto &event = self->m_events.get<web_event::favicon>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, webview *self)
        {
            self->m_events.get<web_event::favicon>().fire(self->favicon());
        };

        const auto id = g_signal_connect(web_view, "notify::favicon", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void saucer::webview::impl::setup<web_event::title>(webview *self)
    {
        auto &event = self->m_events.get<web_event::title>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, webview *self)
        {
            self->m_events.get<web_event::title>().fire(self->page_title());
        };

        const auto id = g_signal_connect(web_view, "notify::title", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(web_view, id); });
    }

    template <>
    void saucer::webview::impl::setup<web_event::load>(webview *)
    {
    }

    std::string webview::impl::inject_script()
    {
        static constexpr auto internal = R"js(
            message: async (message) =>
            {
                window.webkit.messageHandlers.saucer.postMessage(message);
            }
        )js";

        static const auto script = std::format(scripts::webview_script, //
                                               internal,                //
                                               request::stubs());

        return script;
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.message('dom_loaded')";

    std::optional<GValue> convert(std::string_view value)
    {
        static auto regex = std::regex{"^(true|false)|(\\d+)|(.*)$", std::regex::icase};

        std::match_results<std::string_view::iterator> match;

        if (!std::regex_match(value.cbegin(), value.cend(), match, regex))
        {
            return std::nullopt;
        }

        GValue rtn = G_VALUE_INIT;

        if (match[1].matched)
        {
            g_value_init(&rtn, G_TYPE_BOOLEAN);
            g_value_set_boolean(&rtn, value.find_first_of("Tt") != std::string_view::npos);

            return rtn;
        }

        if (match[2].matched)
        {
            guint32 converted{};

            if (std::from_chars(match[2].first, match[2].second, converted).ec != std::errc{})
            {
                return std::nullopt;
            }

            g_value_init(&rtn, G_TYPE_UINT);
            g_value_set_uint(&rtn, converted);

            return rtn;
        }

        g_value_init(&rtn, G_TYPE_STRING);
        g_value_set_string(&rtn, std::string{value}.c_str());

        return rtn;
    }

    WebKitSettings *webview::impl::make_settings(const options &opts)
    {
        std::vector<GValue> values;
        std::vector<std::string> names;

        // TODO: Use some kind of JSON here?

        for (const auto &flag : opts.browser_flags)
        {
            const auto delim = flag.find('=');

            if (delim == std::string::npos)
            {
                continue;
            }

            auto name            = flag.substr(0, delim);
            const auto value     = std::string_view{flag}.substr(delim + 1);
            const auto converted = convert(value);

            if (!converted)
            {
                continue;
            }

            names.emplace_back(std::move(name));
            values.emplace_back(converted.value());
        }

        auto names_ptr = std::views::transform(names, [](auto &value) { return value.c_str(); }) //
                         | std::ranges::to<std::vector<const char *>>();

        // https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/glib/WebKitSettings.cpp#L1753

        return WEBKIT_SETTINGS(g_object_new_with_properties(WEBKIT_TYPE_SETTINGS, values.size(), names_ptr.data(), values.data()));
    }
} // namespace saucer
