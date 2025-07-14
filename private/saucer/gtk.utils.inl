#pragma once

#include "gtk.utils.hpp"

namespace saucer
{
    template <typename T, typename R, typename... Ts, typename Data>
        requires std::same_as<tuple::last_t<std::tuple<Ts...>>, Data *>
    auto utils::connect(T *instance, const char *name, R (*callback)(Ts...), Data *data)
    {
        return g_signal_connect(instance, name, G_CALLBACK(callback), data);
    }
} // namespace saucer
