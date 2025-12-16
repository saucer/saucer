#pragma once

#include "unquoted.hpp"

namespace saucer
{
    namespace detail
    {
        struct unquoted_t
        {
            std::string str;
        };
    } // namespace detail

    template <typename T>
        requires std::constructible_from<std::string, T>
    auto unquoted(T &&value)
    {
        return detail::unquoted_t{std::forward<T>(value)};
    }
} // namespace saucer
