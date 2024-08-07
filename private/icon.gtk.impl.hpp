#pragma once

#include "icon.hpp"
#include "utils.gtk.hpp"

#include <gtk/gtk.h>

namespace saucer
{
    struct icon::impl
    {
        object_ptr<GdkTexture> texture;
    };
} // namespace saucer
