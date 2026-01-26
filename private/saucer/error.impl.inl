#pragma once

#include "error.impl.hpp"

namespace saucer
{
    template <Unwrappable T>
    auto unwrap_safe(T &&value)
    {
        using value_t = std::remove_cvref_t<T>::value_type;

        if (!value.has_value())
        {
            return value_t{};
        }

        return *std::forward<T>(value);
    }
} // namespace saucer
