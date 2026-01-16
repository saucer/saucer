#pragma once

#include "error.hpp"

#include <format>
#include <string_view>

namespace saucer
{
    namespace detail
    {
        template <typename T>
        struct is_expected : std::false_type
        {
        };

        template <typename T, typename E>
        struct is_expected<std::expected<T, E>> : std::true_type
        {
        };

        template <typename T>
        concept Expected = is_expected<T>::value;
    } // namespace detail

    template <Unwrappable T>
    auto unwrap_safe(T &&value)
    {
        using value_t = T::value_type;

        if (!value.has_value())
        {
            return value_t{};
        }

        return *std::forward<T>(value);
    }

    template <typename T>
    auto err(T &&value, std::source_location location)
    {
        using U = std::remove_cvref_t<T>;

        if constexpr (detail::Expected<U>)
        {
            return std::unexpected{value.error()};
        }
        else
        {
            return std::unexpected{error{
                cr::polo<error::impl>{std::in_place_type_t<error::of<U>>{}, std::forward<T>(value)},
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
