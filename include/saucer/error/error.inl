#pragma once

#include "error.hpp"

#include <format>
#include <string_view>

namespace saucer
{
    namespace detail
    {
        template <typename T, typename E>
        struct is_expected : std::false_type
        {
        };

        template <typename T, typename E>
        struct is_expected<std::expected<T, E>, E> : std::true_type
        {
        };
    } // namespace detail

    template <typename T>
    auto err(T &&value, std::source_location location)
    {
        if constexpr (detail::is_expected<std::decay_t<T>, error>::value)
        {
            return std::unexpected{value.error()};
        }
        else
        {
            return std::unexpected{error{
                cr::polo<error::impl>{std::in_place_type_t<error::of<std::decay_t<T>>>{}, std::forward<T>(value)},
                location,
            }};
        }
    }
} // namespace saucer

template <>
struct std::formatter<saucer::error> : std::formatter<std::string_view>
{
    std::format_context::iterator format(const saucer::error &error, std::format_context &ctx) const
    {
        const auto format = std::format("Error at {}:{}: {}", error.location().file_name(), error.location().line(), error.message());
        return std::formatter<string_view>::format(format, ctx);
    }
};
