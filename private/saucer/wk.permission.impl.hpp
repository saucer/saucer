#pragma once

#include "permission.hpp"
#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>

namespace saucer::permission
{
    using block_ptr = utils::objc_obj<void (^)(WKPermissionDecision)>;

    struct request::impl
    {
        utils::objc_ptr<WKFrameInfo> frame;
        block_ptr handler;

      public:
        WKMediaCaptureType type;
    };
} // namespace saucer::permission
