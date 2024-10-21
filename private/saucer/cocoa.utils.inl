#pragma once

#include "cocoa.utils.hpp"

#include <utility>

extern "C"
{
    void *objc_autoreleasePoolPush(void);
    void objc_autoreleasePoolPop(void *pool);
}

namespace saucer
{
    template <typename T>
    T *objc_ptr<T>::retain(T *ptr)
    {
        if (ptr)
        {
            [ptr retain];
        }

        return ptr;
    }

    template <typename T>
    T *objc_ptr<T>::release(T *ptr)
    {
        const autorelease_guard guard{};

        if (ptr)
        {
            [ptr release];
        }

        return ptr;
    }

    template <typename T>
    objc_ptr<T>::objc_ptr() : m_data(nullptr)
    {
    }

    template <typename T>
    objc_ptr<T>::objc_ptr(T *data) : m_data(data)
    {
    }

    template <typename T>
    objc_ptr<T>::objc_ptr(const objc_ptr &other) : m_data(retain(other.m_data))
    {
    }

    template <typename T>
    objc_ptr<T>::objc_ptr(objc_ptr &&other) noexcept : m_data(std::exchange(other.m_data, nullptr))
    {
    }

    template <typename T>
    objc_ptr<T>::~objc_ptr()
    {
        reset(nullptr);
    }

    template <typename T>
    objc_ptr<T> &objc_ptr<T>::operator=(const objc_ptr &other)
    {
        if (this != &other)
        {
            reset(retain(other.m_data));
        }

        return *this;
    }

    template <typename T>
    objc_ptr<T> &objc_ptr<T>::operator=(objc_ptr &&other) noexcept
    {
        if (this != &other)
        {
            reset(std::exchange(other.m_data, nullptr));
        }

        return *this;
    }
    template <typename T>
    T *objc_ptr<T>::get() const
    {
        return m_data;
    }

    template <typename T>
    objc_ptr<T>::operator bool() const
    {
        return m_data != nullptr;
    }

    template <typename T>
    void objc_ptr<T>::reset(T *other)
    {
        release(m_data);
        m_data = other;
    }

    template <typename T>
    objc_ptr<T> objc_ptr<T>::ref(T *data)
    {
        return retain(data);
    }

    inline autorelease_guard::autorelease_guard() : m_pool(objc_autoreleasePoolPush()) {}

    inline autorelease_guard::~autorelease_guard()
    {
        objc_autoreleasePoolPop(m_pool);
    }
} // namespace saucer
