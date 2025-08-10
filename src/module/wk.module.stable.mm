#include "modules/stable/webkit.hpp"

#include "cocoa.app.impl.hpp"
#include "cocoa.icon.impl.hpp"
#include "cocoa.window.impl.hpp"

#include "wk.uri.impl.hpp"
#include "wk.webview.impl.hpp"
#include "wk.permission.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.application = m_impl->platform->application};
    }

    template <>
    natives<window, true> window::native<true>() const
    {
        return {.window = m_impl->platform->window};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->platform->web_view.get()};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
        return {.frame = m_impl->frame.get(), .type = m_impl->type};
    }

    template <>
    natives<uri, true> uri::native<true>() const
    {
        return {.url = m_impl->url.get()};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->icon.get()};
    }
} // namespace saucer
