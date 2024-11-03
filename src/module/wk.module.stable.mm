#include "modules/stable/webkit.hpp"

#include "cocoa.app.impl.hpp"
#include "wk.webview.impl.hpp"
#include "cocoa.window.impl.hpp"

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
        return {.window = m_impl->window};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->web_view.get()};
    }
} // namespace saucer
