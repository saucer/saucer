#pragma once

#include "args.hpp"

namespace saucer
{
    template <typename... Ts>
    std::size_t arguments<Ts...>::size() const
    {
        return std::tuple_size_v<underlying>;
    }

    template <typename... Ts>
    const arguments<Ts...>::underlying &arguments<Ts...>::as_tuple() const
    {
        return static_cast<const underlying &>(*this);
    }

    template <typename... Ts>
    auto make_args(Ts &&...args)
    {
        return arguments<Ts...>{std::forward<Ts>(args)...};
    }
} // namespace saucer
