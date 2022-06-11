#pragma once
#include "serializer.hpp"

namespace saucer
{
    template <typename... T> auto make_arguments(T &&...args)
    {
        return arguments<T...>(std::forward<T>(args)...);
    }
} // namespace saucer