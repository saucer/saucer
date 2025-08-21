#pragma once

#include <saucer/utils/tuple.hpp>

#include "handle.hpp"
#include "ref_obj.hpp"

#include <gtk/gtk.h>

namespace saucer::utils
{
    using g_str_ptr = handle<gchar *, g_free>;

    template <typename T>
    using g_object_ptr = ref_ptr<T, g_object_ref, g_object_unref>;
    using g_bytes_ptr  = ref_ptr<GBytes, g_bytes_ref, g_bytes_unref>;
    using g_event_ptr  = ref_ptr<GdkEvent, gdk_event_ref, gdk_event_unref>;

    template <typename T, typename R, typename... Ts, typename Data>
        requires std::same_as<tuple::last_t<std::tuple<Ts...>>, Data *>
    auto connect(T *instance, const char *name, R (*callback)(Ts...), Data *data);
} // namespace saucer::utils

#include "gtk.utils.inl"
