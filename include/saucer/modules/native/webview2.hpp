#pragma once

#include <saucer/modules/module.hpp>

#include <wrl.h>
#include <windows.h>
#include <WebView2.h>

namespace saucer
{
    using Microsoft::WRL::ComPtr;

    struct natives::window_impl
    {
        HWND hwnd;
    };

    struct natives::webview_impl
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2> web_view;
    };
} // namespace saucer
