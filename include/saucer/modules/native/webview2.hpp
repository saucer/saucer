#pragma once

#include <wrl.h>
#include <windows.h>
#include <WebView2.h>

namespace saucer::native
{
    using Microsoft::WRL::ComPtr;

    struct window
    {
        HWND hwnd;
    };

    struct webview
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2> web_view;
    };
} // namespace saucer::native
