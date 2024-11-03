#pragma once

#include <saucer/modules/module.hpp>

#include <saucer/app.hpp>
#include <saucer/window.hpp>
#include <saucer/webview.hpp>

#include <windows.h>
#include <WebView2.h>

namespace saucer
{
    template <>
    struct stable<application>
    {
        WNDCLASSW wnd_class;
    };

    template <>
    struct stable<window>
    {
        HWND hwnd;
    };

    template <>
    struct stable<webview>
    {
        ICoreWebView2 *webview;
        ICoreWebView2Controller *controller;
    };
} // namespace saucer
