#include "app.hpp"
#include "window.hpp"
#include "webview.hpp"

namespace saucer
{
    template <>
    natives<application, false> application::native<false>() const
    {
        return m_impl.get();
    }

    template <>
    natives<window, false> window::native<false>() const
    {
        return m_impl.get();
    }

    template <>
    natives<webview, false> webview::native<false>() const
    {
        return m_impl.get();
    }

    template <>
    natives<permission::request, false> permission::request::native<false>() const
    {
        return m_impl.get();
    }

    template <>
    natives<url, false> url::native<false>() const
    {
        return m_impl.get();
    }

    template <>
    natives<icon, false> icon::native<false>() const
    {
        return m_impl.get();
    }
} // namespace saucer
