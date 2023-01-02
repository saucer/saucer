#pragma once
#include "serializer.hpp"

namespace saucer
{
    template <typename T> struct is_args : std::false_type
    {
    };

    template <typename... T> struct is_args<arguments<T...>> : std::true_type
    {
        using args_t = std::tuple<T...>;
    };

    template <typename... T> auto make_arguments(T &&...args)
    {
        return arguments<T...>(std::forward<T>(args)...);
    }
} // namespace saucer