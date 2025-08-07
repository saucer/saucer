#pragma once

#include <expected>
#include <system_error>
#include <source_location>

namespace saucer
{
    struct error
    {
        std::error_code error_code;
        std::source_location location;

      public:
        constexpr error(std::error_code, std::source_location = std::source_location::current());
    };

    template <typename T = void>
    using result = std::expected<T, error>;

    template <typename T>
    constexpr std::unexpected<error> err(T);
} // namespace saucer

#include "error.inl"
