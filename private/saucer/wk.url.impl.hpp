#pragma once

#include <saucer/url.hpp>

#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>

namespace saucer
{
    struct url::impl
    {
        utils::objc_ptr<NSURL> url;
    };
} // namespace saucer
