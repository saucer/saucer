#pragma once

#include "utils.gtk.hpp"

#include <utility>

namespace saucer
{

    template <typename T, auto Ref, auto Unref>
    template <auto Action>
    T *ref_ptr<T, Ref, Unref>::perform(T *data)
    {
        if (data)
        {
            std::invoke(Action, data);
        }

        return data;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr() = default;

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(T *data) : m_data(data)
    {
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(const ref_ptr &other) : m_data(other.m_data)
    {
        perform<Ref>(m_data);
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(ref_ptr &&other) noexcept : m_data(std::exchange(other.m_data, nullptr))
    {
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::~ref_ptr()
    {
        perform<Unref>(m_data);
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> &ref_ptr<T, Ref, Unref>::operator=(const ref_ptr &other)
    {
        if (this != &other)
        {
            perform<Unref>(m_data);
            perform<Ref>(other.m_data);

            m_data = other.m_data;
        }

        return *this;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> &ref_ptr<T, Ref, Unref>::operator=(ref_ptr &&other) noexcept
    {
        if (this != &other)
        {
            perform<Unref>(m_data);
            m_data = std::exchange(other.m_data, nullptr);
        }

        return *this;
    }

    template <typename T, auto Ref, auto Unref>
    T *ref_ptr<T, Ref, Unref>::get() const
    {
        return m_data;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::operator bool() const
    {
        return m_data != nullptr;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> ref_ptr<T, Ref, Unref>::copy(T *data)
    {
        perform<Ref>(data);
        return data;
    }
} // namespace saucer
