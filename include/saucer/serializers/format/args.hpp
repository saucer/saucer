#pragma once

#include <tuple>

namespace saucer
{
    template <typename... Ts>
    struct arguments
    {
        std::tuple<Ts...> tuple;
    };

    template <typename... Ts>
    constexpr auto make_args(Ts &&...);
} // namespace saucer

#include "args.inl"
