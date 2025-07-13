#pragma once

#include "window.hpp"

namespace saucer
{
    template <window::event Event, typename T>
    auto window::on(T &&callback)
    {
        setup<Event>();
        return m_events->get<Event>().add(std::forward<T>(callback));
    }

    template <window::event Event, typename T>
    void window::once(T &&callback)
    {
        setup<Event>();
        m_events->get<Event>().once(std::forward<T>(callback));
    }

    template <window::event Event, typename... Ts>
    auto window::await(Ts &&...result)
    {
        setup<Event>();
        return m_events->get<Event>().await(std::forward<Ts>(result)...);
    }
} // namespace saucer
