#include "modules/stable/webview2.hpp"

#include "win32.app.impl.hpp"
#include "wv2.webview.impl.hpp"
#include "win32.window.impl.hpp"
#include "wv2.permission.impl.hpp"

namespace saucer
{
    template <>
    natives<application, true> application::native<true>() const
    {
        return {.wnd_class = m_impl->wnd_class};
    }

    template <>
    natives<window, true> window::native<true>() const
    {
        return {.hwnd = m_impl->hwnd.get()};
    }

    template <>
    natives<webview, true> webview::native<true>() const
    {
        return {.webview = m_impl->web_view.Get(), .controller = m_impl->controller.Get()};
    }

    template <>
    natives<permission::request, true> permission::request::native<true>() const
    {
        return {.request = m_impl->request.Get()};
    }
} // namespace saucer
