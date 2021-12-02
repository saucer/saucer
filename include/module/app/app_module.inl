#pragma once
#include "app_module.hpp"
#include "bridge/bridged_webview.hpp"

namespace saucer
{
    namespace modules
    {
        template <class bridge_t> app<bridge_t>::~app() = default;
        template <class bridge_t> app<bridge_t>::app(bridged_webview<bridge_t> &webview) : module(reinterpret_cast<saucer::webview &>(webview))
        {
            webview.expose("app.hide", [this]() { return m_webview.hide(); });
            webview.expose("app.show", [this]() { return m_webview.show(); });
            webview.expose("app.exit", [this]() { return m_webview.exit(); });

            webview.expose("app.get_size", [this]() { return m_webview.get_size(); });
            webview.expose("app.get_title", [this]() { return m_webview.get_title(); });
            webview.expose("app.get_max_size", [this]() { return m_webview.get_max_size(); });
            webview.expose("app.get_min_size", [this]() { return m_webview.get_min_size(); });
            webview.expose("app.get_dev_tools", [this]() { return m_webview.get_dev_tools(); });
            webview.expose("app.get_decorations", [this]() { return m_webview.get_decorations(); });
            webview.expose("app.get_context_menu", [this]() { return m_webview.get_context_menu(); });
            webview.expose("app.get_always_on_top", [this]() { return m_webview.get_always_on_top(); });

            webview.expose("app.set_dev_tools", [this](bool enabled) { return m_webview.set_dev_tools(enabled); });
            webview.expose("app.set_title", [this](const std::string &title) { return m_webview.set_title(title); });
            webview.expose("app.set_decorations", [this](bool enabled) { return m_webview.set_decorations(enabled); });
            webview.expose("app.set_context_menu", [this](bool enabled) { return m_webview.set_context_menu(enabled); });
            webview.expose("app.set_always_on_top", [this](bool enabled) { return m_webview.set_always_on_top(enabled); });
            webview.expose("app.set_size", [this](std::size_t width, std::size_t height) { return m_webview.set_size(width, height); });
            webview.expose("app.set_max_size", [this](std::size_t width, std::size_t height) { return m_webview.set_min_size(width, height); });
            webview.expose("app.set_min_size", [this](std::size_t width, std::size_t height) { return m_webview.set_max_size(width, height); });
        }
    } // namespace modules
} // namespace saucer