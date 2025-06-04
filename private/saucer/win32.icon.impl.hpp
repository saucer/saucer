#pragma once

#include "icon.hpp"

#include <windows.h>
#include <gdiplus.h>

namespace saucer
{
    struct icon::impl
    {
        std::shared_ptr<Gdiplus::Bitmap> bitmap;
    };
} // namespace saucer
