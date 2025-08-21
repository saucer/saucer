#pragma once

#include <saucer/navigation.hpp>

#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>

namespace saucer
{
    struct navigation::impl
    {
        utils::objc_ptr<WKNavigationAction> action;
    };
} // namespace saucer
