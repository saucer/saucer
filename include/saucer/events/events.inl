#pragma once
#include "events.hpp"

namespace saucer
{
    template <typename Result, typename Iterator, typename... Params>
    callback_iterator<Result, Iterator, Params...>::callback_iterator(Iterator &&iterator, std::tuple<Params...> &params) : m_iterator(iterator), m_params(params)
    {
    }

    template <typename Result, typename Iterator, typename... Params> Result callback_iterator<Result, Iterator, Params...>::operator*()
    {
        return std::apply([this](auto &&...params) { return (m_iterator->second(params...)); }, m_params);
    }

    template <typename Result, typename Iterator, typename... Params> callback_iterator<Result, Iterator, Params...> callback_iterator<Result, Iterator, Params...>::operator++()
    {
        return callback_iterator<Result, Iterator, Params...>(std::move(++m_iterator), m_params);
    }

    template <typename Result, typename Iterator, typename... Params> bool callback_iterator<Result, Iterator, Params...>::operator!=(const callback_iterator &other)
    {
        return other.m_iterator != m_iterator;
    }

    template <typename Type, typename... Params>
    callback_invoker<Type, Params...>::callback_invoker(callbacks_t &&callbacks, std::tuple<Params...> &&params) : m_callbacks(std::move(callbacks)), m_params(params)
    {
    }

    template <typename Type, typename... Params> auto callback_invoker<Type, Params...>::begin()
    {
        return callback_iterator<callback_result_t, raw_iterator_t, Params...>(m_callbacks.begin(), m_params);
    }

    template <typename Type, typename... Params> auto callback_invoker<Type, Params...>::end()
    {
        return callback_iterator<callback_result_t, raw_iterator_t, Params...>(m_callbacks.end(), m_params);
    }

    template <auto Val, typename Type> void event<Val, Type>::clear_callbacks()
    {
        m_callbacks.write()->clear();
    }

    template <auto Val, typename Type> template <typename... Params> typename event<Val, Type>::fire_result_t event<Val, Type>::fire(Params &&...params)
    {
        if constexpr (std::is_same_v<fire_result_t, void>)
        {
            const auto locked_callbacks = m_callbacks.read();
            for (const auto &[id, callback] : *locked_callbacks)
            {
                callback(std::forward<Params>(params)...);
            }
        }
        else
        {
            return fire_result_t(std::move(m_callbacks.copy()), std::tuple<Params...>(std::forward<Params>(params)...));
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
            if constexpr (static_cast<int>(Val) == static_cast<int>(item_t::value))
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

    template <typename... Events> template <typename EventType, std::size_t I> constexpr void event_handler<Events...>::clear([[maybe_unused]] EventType event_type)
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

    template <typename... Events>
    template <typename EventType, std::size_t I>
    constexpr void event_handler<Events...>::unregister([[maybe_unused]] EventType event_type, [[maybe_unused]] std::size_t id)
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