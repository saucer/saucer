#include "modules/stable/webkitgtk.hpp"

#include "gtk.app.impl.hpp"
#include "gtk.icon.impl.hpp"
#include "gtk.window.impl.hpp"

#include "wkg.url.impl.hpp"
#include "wkg.scheme.impl.hpp"
#include "wkg.webview.impl.hpp"
#include "wkg.permission.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.application = m_impl->platform->application.get()};
    }

    template <>
    natives<window, true> window::native<true>() const
    {
        return {.window = m_impl->platform->window.get()};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->platform->web_view};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
        return {.request = m_impl->request.get()};
    }

    template <>
    natives<url, true> url::native<true>() const
    {
        return {.uri = m_impl->uri.get()};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->texture.get()};
    }

    template <>
    natives<stash, true> stash::native<true>() const
    {
        if (m_impl->type() != impl::id_of<scheme::stash_stream>())
        {
            return {};
        }

        return {.stream = G_INPUT_STREAM(static_cast<scheme::stash_stream *>(m_impl.get())->platform->stream.get())};
    }
} // namespace saucer
