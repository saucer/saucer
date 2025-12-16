#pragma once

#include <string>
#include <concepts>

namespace saucer
{
    template <typename T>
        requires std::constructible_from<std::string, T>
    auto unquoted(T &&);
} // namespace saucer

#include "unquoted.inl"
