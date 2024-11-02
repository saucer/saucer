#pragma once

#include "navigation.hpp"

#import <WebKit/WebKit.h>

namespace saucer
{
    struct navigation::impl
    {
        WKNavigationAction *action;
    };
} // namespace saucer
