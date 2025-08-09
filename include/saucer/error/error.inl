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

        inline auto error(std::error_code value)
        {
            return value;
        }

        template <typename T>
        auto error(T &&value)
        {
            return make_error_code(std::forward<T>(value));
        }
    } // namespace detail

    template <typename T>
    auto err(std::expected<T, error> &expected)
    {
        return std::unexpected{expected.error()};
    }

    template <typename T>
    auto err(T &&value, [[maybe_unused]] std::source_location location)
    {
        if constexpr (detail::is_expected<std::remove_cvref_t<T>>::value)
        {
            return std::unexpected{std::forward<T>(value).error()};
        }
        else
        {
            return std::unexpected{error{
                .error_code = detail::error(std::forward<T>(value)),
                .location   = location,
            }};
        }
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
