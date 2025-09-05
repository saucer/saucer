#pragma once

#include <concepts>

namespace saucer::utils
{
    namespace detail
    {
        template <typename T>
        struct callback;
    }

    template <typename Callback, typename T, typename... Ts>
        requires std::invocable<Callback, Ts...>
    constexpr auto invoke(Callback &&, T *, Ts &&...);

    template <detail::callback Callback, typename T, typename... Ts>
    constexpr auto invoke(T *, Ts &&...);
} // namespace saucer::utils

#include "invoke.inl"
