#pragma once

#include "args.hpp"

namespace saucer
{
    template <typename... Ts>
    std::size_t arguments<Ts...>::size() const
    {
        return sizeof...(Ts);
    }

    template <typename... Ts>
    const std::tuple<Ts...> &arguments<Ts...>::as_tuple() const
    {
        return static_cast<const std::tuple<Ts...> &>(*this);
    }

    template <typename... Ts>
    auto make_args(Ts &&...args)
    {
        return arguments<Ts...>{std::forward<Ts>(args)...};
    }
} // namespace saucer
