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
        return std::visit([](auto &data) { return data.data(); }, m_data);
    }

    template <typename T>
    std::size_t stash<T>::size() const
    {
        return std::visit([](auto &data) { return data.size(); }, m_data);
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
} // namespace saucer
