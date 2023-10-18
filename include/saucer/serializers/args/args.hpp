#pragma once
#include <tuple>

namespace saucer
{
    template <typename... T>
    struct arguments : public std::tuple<T...>
    {
        using std::tuple<T...>::tuple;
        using tuple_t = std::tuple<T...>;
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
