#pragma once

#include "../utils/tuple.hpp"

namespace saucer::traits
{
    template <typename T>
    struct function_traits;

    template <typename T, typename R>
    struct mock_return;

    template <typename T, typename Args, typename Executor>
    struct transformer;

    template <typename T>
    struct resolver;
} // namespace saucer::traits

#include "traits.inl"
