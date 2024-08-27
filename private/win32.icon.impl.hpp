#pragma once

#include "icon.hpp"

#include <wrl.h>
#include <gdiplus.h>

namespace saucer
{
    using Microsoft::WRL::ComPtr;

    struct icon::impl
    {
        std::shared_ptr<Gdiplus::Bitmap> bitmap;
    };
} // namespace saucer
