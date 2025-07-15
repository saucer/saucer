#pragma once

#include "window.hpp"

namespace saucer
{
    template <window::event Event>
    auto window::on(events::event<Event>::listener listener)
    {
        setup<Event>();
        m_events->get<Event>().add(std::move(listener));
    }

    template <window::event Event>
    void window::once(events::event<Event>::listener::callback cb)
    {
        setup<Event>();
        m_events->get<Event>().once(std::move(cb));
    }

    template <window::event Event, typename... Ts>
    auto window::await(Ts &&...result)
    {
        setup<Event>();
        return m_events->get<Event>().await(std::forward<Ts>(result)...);
    }
} // namespace saucer
