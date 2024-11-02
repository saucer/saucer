#include "modules/stable/webkitgtk.hpp"

#include "gtk.app.impl.hpp"
#include "gtk.window.impl.hpp"
#include "wkg.webview.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.application = m_impl->application};
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
} // namespace saucer
