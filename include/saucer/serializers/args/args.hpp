#pragma once

#include <tuple>

namespace saucer
{
    template <typename... T>
    struct arguments : std::tuple<T...>
    {
        using std::tuple<T...>::tuple;
        using underlying = std::tuple<T...>;
    };

    template <typename T>
    concept is_arguments = requires(T args) {
        []<typename... A>(arguments<A...> &) {
        }(args);
    };

    template <typename... T>
    auto make_args(T &&...);
} // namespace saucer

#include "args.inl"
