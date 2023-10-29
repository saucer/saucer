#pragma once

#include "args.hpp"

namespace saucer
{
    template <typename... T>
    auto make_args(T &&...args)
    {
        return arguments<T...>(std::forward<T>(args)...);
    }
} // namespace saucer
