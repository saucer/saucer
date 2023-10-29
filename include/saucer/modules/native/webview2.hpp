#pragma once

#include <wrl.h>
#include <windows.h>
#include <WebView2.h>

namespace saucer::native
{
    struct window
    {
        HWND hwnd;
    };

    struct webview
    {
        Microsoft::WRL::ComPtr<ICoreWebView2> web_view;
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller;
    };
} // namespace saucer::native
