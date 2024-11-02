#pragma once

#include "ref_ptr.hpp"

#include <utility>

namespace saucer::utils
{
    template <typename T, auto Ref, auto Unref>
    template <auto Action>
    T *ref_ptr<T, Ref, Unref>::perform(T *ptr)
    {
        if (ptr)
        {
            Action(ptr);
        }

        return ptr;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr() = default;

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(T *ptr) : m_ptr(ptr)
    {
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(const ref_ptr &other) : m_ptr(perform<Ref>(other.m_ptr))
    {
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::ref_ptr(ref_ptr &&other) noexcept : m_ptr(std::exchange(other.m_ptr, nullptr))
    {
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::~ref_ptr()
    {
        reset();
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> &ref_ptr<T, Ref, Unref>::operator=(const ref_ptr &other)
    {
        if (this != &other)
        {
            reset(perform<Ref>(other.m_ptr));
        }

        return *this;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> &ref_ptr<T, Ref, Unref>::operator=(ref_ptr &&other) noexcept
    {
        std::swap(m_ptr, other.m_ptr);
        return *this;
    }

    template <typename T, auto Ref, auto Unref>
    T *ref_ptr<T, Ref, Unref>::get() const
    {
        return m_ptr;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref>::operator bool() const
    {
        return m_ptr != nullptr;
    }

    template <typename T, auto Ref, auto Unref>
    void ref_ptr<T, Ref, Unref>::reset(T *ptr)
    {
        perform<Unref>(m_ptr);
        m_ptr = ptr;
    }

    template <typename T, auto Ref, auto Unref>
    ref_ptr<T, Ref, Unref> ref_ptr<T, Ref, Unref>::ref(T *data)
    {
        return perform<Ref>(data);
    }
} // namespace saucer::utils
