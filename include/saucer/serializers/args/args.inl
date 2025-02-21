#pragma once

#include "args.hpp"

namespace saucer
{
    namespace impl
    {
        template <typename T>
        struct is_arguments : std::false_type
        {
        };

        template <typename... Ts>
        struct is_arguments<arguments<Ts...>> : std::true_type
        {
        };
    } // namespace impl

    template <typename... Ts>
    arguments<Ts...>::arguments(Ts... args) : m_tuple{std::move(args)...}
    {
    }

    template <typename... Ts>
    constexpr auto &arguments<Ts...>::tuple()
    {
        return m_tuple;
    }

    template <typename... Ts>
    constexpr std::size_t arguments<Ts...>::size()
    {
        return sizeof...(Ts);
    }

    template <typename... Ts>
    constexpr auto make_args(Ts &&...args)
    {
        return arguments<Ts...>{std::forward<Ts>(args)...};
    }
} // namespace saucer
