#include "modules/stable/webkitgtk.hpp"

#include "gtk.app.impl.hpp"
#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"

#include "wkg.uri.impl.hpp"
#include "wkg.webview.impl.hpp"
#include "wkg.permission.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.application = m_impl->application.get()};
    }

    template <>
    natives<window, true> window::native<true>() const
    {
        return {.window = m_impl->window.get()};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->web_view};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
        return {.request = m_impl->request.get()};
    }

    template <>
    natives<uri, true> uri::native<true>() const
    {
        return {.uri = m_impl->uri.get()};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->texture.get()};
    }
} // namespace saucer
