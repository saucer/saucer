#pragma once

#include "cstring.hpp"

#include <cassert>
#include <concepts>
#include <type_traits>

namespace saucer
{
    template <typename T>
    struct is_cstring_like : std::false_type
    {
    };

    template <typename T>
        requires requires(T &value) {
            { value.c_str() } -> std::same_as<const typename std::remove_cvref_t<T>::value_type *>;
        }
    struct is_cstring_like<T> : std::true_type
    {
    };

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::basic_cstring_view() noexcept : m_size(0), m_data(nullptr)
    {
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::basic_cstring_view(const T *data) noexcept : m_size(Traits::length(data)), m_data(data)
    {
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::basic_cstring_view(const T *data, std::size_t size) noexcept : m_size(size), m_data(data)
    {
    }

    template <typename T, typename Traits>
    template <cstring_like U>
    constexpr basic_cstring_view<T, Traits>::basic_cstring_view(U &&other)
        requires(not std::same_as<std::remove_cvref_t<U>, basic_cstring_view>)
        : basic_cstring_view(std::forward<U>(other).c_str())
    {
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_iterator basic_cstring_view<T, Traits>::begin() const noexcept
    {
        return cbegin();
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_iterator basic_cstring_view<T, Traits>::end() const noexcept
    {
        return cend();
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_iterator basic_cstring_view<T, Traits>::cbegin() const noexcept
    {
        return m_data;
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_iterator basic_cstring_view<T, Traits>::cend() const noexcept
    {
        return m_data + size();
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_reference basic_cstring_view<T, Traits>::at(std::size_t pos) const
    {
        assert(pos < size());
        return m_data[pos];
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_reference basic_cstring_view<T, Traits>::operator[](std::size_t pos) const
    {
        return m_data[pos];
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_pointer basic_cstring_view<T, Traits>::data() const noexcept
    {
        return m_data;
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::const_pointer basic_cstring_view<T, Traits>::c_str() const noexcept
    {
        return m_data;
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::size_type basic_cstring_view<T, Traits>::size() const noexcept
    {
        return m_size;
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::size_type basic_cstring_view<T, Traits>::length() const noexcept
    {
        return size();
    }

    template <typename T, typename Traits>
    constexpr basic_cstring_view<T, Traits>::operator std::basic_string_view<T, Traits>() const noexcept
    {
        return m_data;
    }
} // namespace saucer
