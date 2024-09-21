#include "wkg.webview.impl.hpp"

#include "scripts.hpp"

#include <regex>
#include <optional>

#include <fmt/core.h>

namespace saucer
{
    template <>
    void saucer::webview::impl::setup<web_event::title_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::title_changed>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, webview *self)
        {
            self->m_events.at<web_event::title_changed>().fire(self->page_title());
        };

        g_signal_connect(self->m_impl->web_view, "notify::title", G_CALLBACK(+callback), self);
    }

    template <>
    void saucer::webview::impl::setup<web_event::load_finished>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::icon_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::icon_changed>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, webview *self)
        {
            self->m_events.at<web_event::icon_changed>().fire(self->favicon());
        };

        g_signal_connect(self->m_impl->web_view, "notify::favicon", G_CALLBACK(+callback), self);
    }

    template <>
    void saucer::webview::impl::setup<web_event::load_started>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::url_changed>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

    const std::string &webview::impl::inject_script()
    {
        static std::optional<std::string> instance;

        if (instance)
        {
            return instance.value();
        }

        instance.emplace(fmt::format(scripts::webview_script, fmt::arg("internal", R"js(
        send_message: async (message) =>
        {
            window.webkit.messageHandlers.saucer.postMessage(message);
        }
        )js")));

        return instance.value();
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.send_message('dom_loaded')";

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

    WebKitSettings *webview::impl::make_settings(const preferences &prefs)
    {
        std::vector<GValue> values;
        std::vector<std::string> names;

        for (const auto &flag : prefs.browser_flags)
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

        auto names_ptr = std::views::transform(names, [](auto &value) { return value.c_str(); }) |
                         std::ranges::to<std::vector<const char *>>();

        // https://opensource.apple.com/source/WebKit2/WebKit2-7601.1.56/UIProcess/API/gtk/WebKitSettings.cpp.auto.html

        return WEBKIT_SETTINGS(
            g_object_new_with_properties(WEBKIT_TYPE_SETTINGS, values.size(), names_ptr.data(), values.data()));
    }
} // namespace saucer
