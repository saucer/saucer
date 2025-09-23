#include "modules/stable/qt.hpp"

#include "qt.app.impl.hpp"
#include "qt.icon.impl.hpp"
#include "qt.window.impl.hpp"

#include "qt.url.impl.hpp"
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
        return {.window = m_impl->platform->window.get()};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->platform->web_view.get()};
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
    natives<url, true> url::native<true>() const
    {
        return {.url = m_impl->url};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->icon};
    }
} // namespace saucer
