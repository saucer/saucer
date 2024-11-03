#pragma once

#include <saucer/modules/module.hpp>

#include <saucer/app.hpp>
#include <saucer/window.hpp>
#include <saucer/webview.hpp>

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

namespace saucer
{
    template <>
    struct stable<application>
    {
        NSApplication *application;
    };

    template <>
    struct stable<window>
    {
        NSWindow *window;
    };

    template <>
    struct stable<webview>
    {
        WKWebView *webview;
    };
} // namespace saucer
