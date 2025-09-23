#pragma once

#include "webview.hpp"
#include "traits/traits.hpp"

namespace saucer
{
    template <typename T>
    void webview::handle(const std::string &name, T &&handler)
    {
        using transformer = traits::transformer<T, std::tuple<scheme::request>, scheme::executor>;
        handle(name, scheme::resolver{transformer::transform(std::forward<T>(handler))});
    }

    template <webview::event Event>
    auto webview::on(events::event<Event>::listener listener)
    {
        setup<Event>();
        return m_events->get<Event>().add(std::move(listener));
    }

    template <webview::event Event>
    void webview::once(events::event<Event>::listener::callback cb)
    {
        setup<Event>();
        return m_events->get<Event>().once(std::move(cb));
    }

    template <webview::event Event, typename... Ts>
    auto webview::await(Ts &&...result)
    {
        setup<Event>();
        return m_events->get<Event>().await(std::forward<Ts>(result)...);
    }
} // namespace saucer
