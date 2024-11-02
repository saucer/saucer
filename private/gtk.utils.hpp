#pragma once

#include "ref_ptr.hpp"

#include <gtk/gtk.h>

namespace saucer::utils
{
    template <typename T>
    using g_object_ptr = ref_ptr<T, g_object_ref, g_object_unref>;
    using g_bytes_ptr  = ref_ptr<GBytes, g_bytes_ref, g_bytes_unref>;
    using g_event_ptr  = ref_ptr<GdkEvent, gdk_event_ref, gdk_event_unref>;
} // namespace saucer::utils
