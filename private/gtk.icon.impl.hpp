#pragma once

#include "icon.hpp"
#include "gtk.utils.hpp"

#include <gtk/gtk.h>

namespace saucer
{
    struct icon::impl
    {
        g_object_ptr<GdkTexture> texture;
    };
} // namespace saucer
