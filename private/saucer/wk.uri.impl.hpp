#pragma once

#include <saucer/uri.hpp>

#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>

namespace saucer
{
    struct uri::impl
    {
        utils::objc_ptr<NSURL> url;
    };
} // namespace saucer
