#pragma once

#include "win32.utils.hpp"

namespace saucer::utils
{
    template <typename T, auto Release>
    win_handle<T, Release>::win_handle() = default;

    template <typename T, auto Release>
    win_handle<T, Release>::win_handle(T handle) : m_handle(handle)
    {
    }

    template <typename T, auto Release>
    win_handle<T, Release>::win_handle(win_handle &&other) noexcept : m_handle(std::exchange(other.m_handle, nullptr))
    {
    }

    template <typename T, auto Release>
    win_handle<T, Release>::~win_handle()
    {
        reset(nullptr);
    }

    template <typename T, auto Release>
    win_handle<T, Release> &win_handle<T, Release>::operator=(win_handle &&other) noexcept
    {
        if (this != &other)
        {
            reset(std::exchange(other.m_handle, nullptr));
        }

        return *this;
    }

    template <typename T, auto Release>
    const T &win_handle<T, Release>::get() const
    {
        return m_handle;
    }

    template <typename T, auto Release>
    T &win_handle<T, Release>::reset(T other)
    {
        if (m_handle)
        {
            Release(m_handle);
        }

        m_handle = other;

        return m_handle;
    }
} // namespace saucer::utils
