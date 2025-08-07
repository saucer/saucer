#pragma once

#include "error.hpp"

#include <format>
#include <string_view>

namespace saucer
{
    constexpr error::error(std::error_code error_code, std::source_location location) : error_code(error_code), location(location) {}

    template <typename T>
    constexpr std::unexpected<error> err(T value)
    {
        return std::unexpected<error>{make_error_code(value)};
    }

    constexpr std::unexpected<error> err(std::error_code value)
    {
        return std::unexpected<error>{value};
    }

    template <typename T>
    constexpr std::unexpected<error> err(std::expected<T, error> value)
    {
        return std::unexpected<error>{value.error()};
    }
} // namespace saucer

template <>
struct std::formatter<saucer::error> : std::formatter<std::string_view>
{
    std::format_context::iterator format(const saucer::error &error, std::format_context &ctx) const
    {
        auto [code, loc] = error;
        auto temp        = std::format("Error at {}:{}: {} ({})", loc.file_name(), loc.line(), code.message(), code.category().name());

        return std::formatter<string_view>::format(temp, ctx);
    }
};
