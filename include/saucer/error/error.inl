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

    template <>
    struct err<void> : error
    {
        err(error value) : error{std::move(value)} {}
    };
    err(error) -> err<void>;

    template <typename T, typename E>
    struct err<std::expected<T, E>> : std::unexpected<E>
    {
        template <detail::Expected U>
        err(U &&value) : std::unexpected<E>{std::forward<U>(value).error()}
        {
        }
    };
    template <detail::Expected T>
    err(T &&) -> err<std::remove_cvref_t<T>>;

    template <typename T, typename... Ts>
    struct err : std::unexpected<error>
    {
        static auto make(std::source_location location, T value, Ts... params)
        {
            auto rtn     = saucer::error::of<std::remove_cvref_t<T>>{}(std::forward<T>(value), std::forward<Ts>(params)...);
            rtn.location = location;

            return rtn;
        }

      public:
        err(T value, Ts... params, std::source_location location = std::source_location::current())
            : std::unexpected<saucer::error>{make(location, std::forward<T>(value), std::forward<Ts>(params)...)}
        {
        }
    };
    template <typename T, typename... Ts>
    err(T &&, Ts &&...) -> err<T, Ts...>;
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
