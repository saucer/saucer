#pragma once
#include "constants.hpp"

#include <stdexcept>
#include <string_view>

namespace saucer
{
    template <bool Enable = enable_assert> constexpr void c_assert(std::string_view message)
    {
        if constexpr (Enable)
        {
            throw std::logic_error(std::string{message});
            std::terminate();
        }
    }
} // namespace saucer