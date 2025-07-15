#pragma once

#include <type_traits>

namespace saucer
{
    template <typename T>
    struct stable_natives;

    template <typename T, bool Stable>
    using natives = std::conditional_t<Stable, stable_natives<T>, typename T::impl *>;
} // namespace saucer
