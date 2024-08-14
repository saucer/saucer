#pragma once

#include "icon.hpp"

#import <Cocoa/Cocoa.h>

namespace saucer
{
    struct icon::impl
    {
        NSImage *icon;
    };
} // namespace saucer
