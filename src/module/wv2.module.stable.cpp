#include "modules/stable/webview2.hpp"

#include "win32.app.impl.hpp"
#include "win32.icon.impl.hpp"
#include "win32.window.impl.hpp"

#include "wv2.url.impl.hpp"
#include "wv2.webview.impl.hpp"
#include "wv2.permission.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.wnd_class = m_impl->platform->wnd_class};
    }

    template <>
    natives<window, true> window::native<true>() const
    {
        return {.hwnd = m_impl->platform->hwnd.get()};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->platform->web_view.Get(), .controller = m_impl->platform->controller.Get()};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
        return {.request = m_impl->request.Get()};
    }

    template <>
    natives<url, true> url::native<true>() const
    {
        return {.url = &m_impl->url, .components = &m_impl->components};
    }

    template <>
    natives<icon, true> icon::native<true>() const
    {
        return {.icon = m_impl->bitmap.get()};
    }
} // namespace saucer
