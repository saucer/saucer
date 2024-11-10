#pragma once

#include "ref_obj.hpp"

#include <utility>

namespace saucer::utils
{
    template <typename T, auto Ref, auto Unref, T Empty>
    template <auto Action>
    T ref_obj<T, Ref, Unref, Empty>::perform(T value)
    {
        if (value != Empty)
        {
            Action(value);
        }

        return value;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::ref_obj() = default;

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::ref_obj(T value) : m_value(value)
    {
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::ref_obj(const ref_obj &other) : m_value(perform<Ref>(other.m_value))
    {
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::ref_obj(ref_obj &&other) noexcept : m_value(std::exchange(other.m_value, Empty))
    {
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::~ref_obj()
    {
        reset();
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty> &ref_obj<T, Ref, Unref, Empty>::operator=(const ref_obj &other)
    {
        if (this != &other)
        {
            reset(perform<Ref>(other.m_value));
        }

        return *this;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty> &ref_obj<T, Ref, Unref, Empty>::operator=(ref_obj &&other) noexcept
    {
        std::swap(m_value, other.m_value);
        return *this;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    T ref_obj<T, Ref, Unref, Empty>::get() const
    {
        return m_value;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty>::operator bool() const
    {
        return m_value != Empty;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    void ref_obj<T, Ref, Unref, Empty>::reset(T value)
    {
        perform<Unref>(m_value);
        m_value = value;
    }

    template <typename T, auto Ref, auto Unref, T Empty>
    ref_obj<T, Ref, Unref, Empty> ref_obj<T, Ref, Unref, Empty>::ref(T value)
    {
        return perform<Ref>(value);
    }
} // namespace saucer::utils
