#pragma once

#include <string>
#include <string_view>

namespace saucer
{
    template <typename T>
    struct is_cstring_like;

    template <typename T>
    concept cstring_like = is_cstring_like<T>::value;

    template <typename T, typename Traits = std::char_traits<T>>
    struct basic_cstring_view
    {
        using traits_type     = Traits;
        using value_type      = T;
        using pointer         = value_type *;
        using const_pointer   = const value_type *;
        using reference       = T &;
        using const_reference = const T &;
        using const_iterator  = const_pointer;
        using iterator        = const_iterator;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

      private:
        const_pointer m_data;

      public:
        constexpr basic_cstring_view() noexcept;
        constexpr basic_cstring_view(const T *) noexcept;

      public:
        template <cstring_like U>
        constexpr basic_cstring_view(U &&)
            requires(not std::same_as<std::remove_cvref_t<U>, basic_cstring_view>);

      public:
        constexpr basic_cstring_view(const basic_cstring_view &)            = default;
        constexpr basic_cstring_view &operator=(const basic_cstring_view &) = default;

      public:
        constexpr const_iterator begin() const noexcept;
        constexpr const_iterator end() const noexcept;

      public:
        constexpr const_iterator cbegin() const noexcept;
        constexpr const_iterator cend() const noexcept;

      public:
        constexpr const_reference at(size_type) const;
        constexpr const_reference operator[](size_type) const;

      public:
        constexpr const_pointer data() const noexcept;
        constexpr const_pointer c_str() const noexcept;

      public:
        [[nodiscard]] constexpr size_type size() const noexcept;
        [[nodiscard]] constexpr size_type length() const noexcept;

      public:
        constexpr operator std::basic_string_view<T, Traits>() const noexcept;
    };

    using cstring_view  = basic_cstring_view<char>;
    using cwstring_view = basic_cstring_view<wchar_t>;
} // namespace saucer

#include "cstring.inl"
