#pragma once

#include <saucer/modules/module.hpp>

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

namespace saucer
{
    struct natives::window_impl
    {
        NSWindow *window;
    };

    struct natives::webview_impl
    {
        WKWebView *web_view;
    };
} // namespace saucer
