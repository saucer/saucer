#pragma once

#include <saucer/modules/module.hpp>

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

namespace saucer
{
    struct window::impl
    {
        NSWindow *window;
    };

    struct webview::impl
    {
        WKWebView *web_view;
    };
} // namespace saucer
