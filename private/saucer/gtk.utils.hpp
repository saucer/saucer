#pragma once

#include <saucer/utils/tuple.hpp>

#include "handle.hpp"
#include "ref_obj.hpp"

#include <gtk/gtk.h>

namespace saucer::utils
{
    template <typename T>
    void widget_unref(T *);

    using g_str_ptr   = handle<gchar *, g_free>;
    using g_error_ptr = handle<GError *, g_error_free>;
    using g_bytes_ptr = ref_ptr<GBytes, g_bytes_ref, g_bytes_unref>;
    using g_event_ptr = ref_ptr<GdkEvent, gdk_event_ref, gdk_event_unref>;

    template <typename T>
    using g_object_ptr = ref_ptr<T, g_object_ref, g_object_unref>;

    template <typename T>
    using g_widget_ptr = ref_ptr<T, g_object_ref, widget_unref<T>>;

    template <typename T, typename R, typename... Ts, typename Data>
        requires std::same_as<tuple::last_t<std::tuple<Ts...>>, Data *>
    auto connect(T *instance, const char *name, R (*callback)(Ts...), Data *data);
} // namespace saucer::utils

#include "gtk.utils.inl"
