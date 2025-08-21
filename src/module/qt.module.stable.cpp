#include "modules/stable/qt.hpp"

#include "qt.app.impl.hpp"
#include "qt.icon.impl.hpp"
#include "qt.window.impl.hpp"

#include "qt.uri.impl.hpp"
#include "qt.webview.impl.hpp"
#include "qt.permission.impl.hpp"

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
        return {.window = m_impl->platform->window};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->platform->web_view};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
#ifdef SAUCER_QT6
        return {.request = m_impl->request};
#else
        return {};
#endif
    }

    template <>
    natives<uri, true> uri::native<true>() const
    {
        return {.uri = m_impl->uri};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->icon};
    }
} // namespace saucer
