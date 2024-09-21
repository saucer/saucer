#pragma once

#include "required.hpp"

#include <utility>

namespace saucer
{
    template <typename T>
    template <typename... Ts>
    required<T>::required(Ts &&...args) : m_value(std::forward<Ts>(args)...)
    {
    }

    template <typename T>
    T &required<T>::value()
    {
        return m_value;
    }

    template <typename T>
    const T &required<T>::value() const
    {
        return m_value;
    }

    template <typename T>
    T *required<T>::operator->()
    {
        return &m_value;
    }

    template <typename T>
    const T *required<T>::operator->() const
    {
        return &m_value;
    }
} // namespace saucer
