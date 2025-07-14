#pragma once

#include "webview.hpp"
#include "traits/traits.hpp"

namespace saucer
{
    template <typename T>
    void webview::handle_scheme(const std::string &name, T &&handler)
    {
        using transformer = traits::transformer<T, std::tuple<scheme::request>, scheme::executor>;
        handle_scheme(name, scheme::resolver{transformer::transform(std::forward<T>(handler))});
    }

    template <webview::event Event, typename T>
    void webview::once(T &&callback)
    {
        setup<Event>();
        m_events->get<Event>().once(std::forward<T>(callback));
    }

    template <webview::event Event, typename T>
    auto webview::on(T &&callback)
    {
        setup<Event>();
        m_events->get<Event>().add(std::forward<T>(callback));
    }

    template <webview::event Event, typename... Ts>
    auto webview::await(Ts &&...result)
    {
        setup<Event>();
        return m_events->get<Event>().await(std::forward<Ts>(result)...);
    }
} // namespace saucer
