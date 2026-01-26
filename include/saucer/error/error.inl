#pragma once

#include "error.hpp"

#include <format>
#include <string_view>

namespace saucer
{
    template <typename T>
    struct detail::is_expected : std::false_type
    {
    };

    template <typename T, typename E>
    struct detail::is_expected<std::expected<T, E>> : std::true_type
    {
    };

    inline auto err(error value)
    {
        return std::unexpected{std::move(value)};
    }

    template <typename T>
        requires(detail::Expected<std::remove_cvref_t<T>>)
    auto err(T &&value)
    {
        return std::unexpected{std::forward<T>(value).error()};
    }

    template <typename T>
        requires(not detail::Expected<std::remove_cvref_t<T>>)
    auto err(T &&value, std::source_location location)
    {
        auto rtn     = saucer::error::of<std::remove_cvref_t<T>>{}(std::forward<T>(value));
        rtn.location = location;

        return std::unexpected{std::move(rtn)};
    }
} // namespace saucer

template <>
struct std::formatter<saucer::error> : std::formatter<std::string_view>
{
    std::format_context::iterator format(const saucer::error &error, std::format_context &ctx) const
    {
        const auto format = std::format("Error at {}:{}: {}", error.location.file_name(), error.location.line(), error.message);
        return std::formatter<string_view>::format(format, ctx);
    }
};
