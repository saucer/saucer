#pragma once
#include "events.hpp"

namespace saucer
{
    template <auto Val, typename Type> void event<Val, Type>::clear_callbacks()
    {
        m_callbacks.write()->clear();
    }

    template <auto Val, typename Type> template <typename... Params> typename event<Val, Type>::fire_result_t event<Val, Type>::fire(Params &&...params)
    {
        const auto locked_callbacks = m_callbacks.read();

        if constexpr (std::is_same_v<fire_result_t, void>)
        {
            for (const auto &[id, callback] : *locked_callbacks)
            {
                callback(std::forward<Params>(params)...);
            }
        }
        else
        {
            fire_result_t rtn;
            for (const auto &[id, callback] : *locked_callbacks)
            {
                rtn.emplace_back(callback(std::forward<Params>(params)...));
            }
            return rtn;
        }
    }

    template <auto Val, typename Type> std::size_t event<Val, Type>::add_callback(callback_t &&callback)
    {
        m_callbacks.write()->emplace(++m_counter, std::move(callback));
        return m_counter;
    }

    template <auto Val, typename Type> void event<Val, Type>::remove_callback(const std::size_t &id)
    {
        m_callbacks.write()->erase(id);
    }

    template <typename... Events> template <auto Val, std::size_t I> constexpr auto event_handler<Events...>::get()
    {
        constexpr auto size = std::tuple_size_v<tuple_t>;
        if constexpr (I < size)
        {
            using item_t = std::tuple_element_t<I, tuple_t>;
            if constexpr (Val == item_t::value)
            {
                return type_identity<typename item_t::callback_t>();
            }
            else
            {
                return get<Val, I + 1>();
            }
        }
        else
        {
            return type_identity<void>();
        }
    }

    template <typename... Events> template <auto Val, std::size_t I> constexpr auto &event_handler<Events...>::at()
    {
        constexpr auto size = std::tuple_size_v<tuple_t>;
        if constexpr (I < size)
        {
            using item_t = std::tuple_element_t<I, tuple_t>;
            if constexpr (Val == item_t::value)
            {
                return std::get<I>(events);
            }
            else
            {
                return at<Val, I + 1>();
            }
        }
    }
    template <typename... Events> template <typename EventType, std::size_t I> constexpr void event_handler<Events...>::clear(EventType event_type)
    {
        constexpr auto size = std::tuple_size_v<tuple_t>;
        if constexpr (I < size)
        {
            using element_t = std::tuple_element_t<I, tuple_t>;

            if (element_t::value == event_type)
            {
                std::get<I>(events).clear_callbacks();
            }
            else
            {
                clear<EventType, I + 1>(event_type);
            }
        }
    }
    template <typename... Events> template <typename EventType, std::size_t I> constexpr void event_handler<Events...>::unregister(EventType event_type, std::size_t id)
    {
        constexpr auto size = std::tuple_size_v<tuple_t>;
        if constexpr (I < size)
        {
            using element_t = std::tuple_element_t<I, tuple_t>;

            if (element_t::value == event_type)
            {
                std::get<I>(events).remove_callback(id);
            }
            else
            {
                unregister<EventType, I + 1>(event_type, id);
            }
        }
    }
} // namespace saucer