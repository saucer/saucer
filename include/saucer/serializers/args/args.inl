#pragma once

#include "args.hpp"

namespace saucer
{
    template <typename... Ts>
    constexpr std::size_t arguments<Ts...>::size() const
    {
        return sizeof...(Ts);
    }

    template <typename... Ts>
    constexpr const arguments<Ts...>::tuple &arguments<Ts...>::as_tuple() const
    {
        return static_cast<const std::tuple<Ts...> &>(*this);
    }

    template <typename... Ts>
    constexpr auto make_args(Ts &&...args)
    {
        return arguments<Ts...>{std::forward<Ts>(args)...};
    }
} // namespace saucer
