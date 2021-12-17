#pragma once
#include "promise.hpp"

namespace saucer
{
    template <typename T> promise<T>::~promise() = default;

    template <typename T> void promise<T>::resolve(const T &value)
    {
        if (*m_callback.read())
        {
            (*m_callback.read())(value);
        }
    }

    template <typename T> void promise<T>::then(const callback_t &callback)
    {
        m_callback.assign(callback);
    }

    inline promise<void>::~promise() = default;

    inline void promise<void>::resolve()
    {
        if (*m_callback.read())
        {
            (*m_callback.read())();
        }
    }

    inline void promise<void>::then(const callback_t &callback)
    {
        m_callback.assign(callback);
    }
} // namespace saucer