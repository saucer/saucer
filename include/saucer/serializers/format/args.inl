#pragma once

#include "args.hpp"

namespace saucer
{
    template <typename... Ts>
    constexpr auto make_args(Ts &&...args)
    {
        return arguments{std::make_tuple(std::forward<Ts>(args)...)};
    }
} // namespace saucer
