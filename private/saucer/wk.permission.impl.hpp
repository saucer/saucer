#pragma once

#include "permission.hpp"
#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>

namespace saucer::permission
{
    using block_t = void (^)(WKPermissionDecision);

    struct request::impl
    {
        utils::objc_ptr<WKFrameInfo> frame;
        utils::objc_obj<block_t> handler;

      public:
        WKMediaCaptureType type;
    };
} // namespace saucer::permission
