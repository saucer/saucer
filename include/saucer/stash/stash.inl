#pragma once

#include "stash.hpp"

namespace saucer
{
    template <typename T>
    stash<T>::stash(variant_t data) : m_data(std::move(data))
    {
    }

    template <typename T>
    T *stash<T>::data() const
    {
        auto visitor = [](auto &data)
        {
            return data.data();
        };

        if (std::holds_alternative<lazy_t>(m_data))
        {
            return std::visit(visitor, std::get<lazy_t>(m_data).get());
        }

        return std::visit(visitor, std::get<data_t>(m_data));
    }

    template <typename T>
    std::size_t stash<T>::size() const
    {
        auto visitor = [](auto &data)
        {
            return data.size();
        };

        if (std::holds_alternative<lazy_t>(m_data))
        {
            return std::visit(visitor, std::get<lazy_t>(m_data).get());
        }

        return std::visit(visitor, std::get<data_t>(m_data));
    }

    template <typename T>
    stash<T> stash<T>::from(owning_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    stash<T> stash<T>::view(viewing_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    stash<T> stash<T>::lazy(lazy_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    template <typename Callback>
    stash<T> stash<T>::lazy(Callback &&callback)
    {
        return {std::async(std::launch::deferred, std::forward<Callback>(callback)).share()};
    }
} // namespace saucer
