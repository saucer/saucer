#pragma once

#include <saucer/win32.utils.hpp>
#include <saucer/modules/module.hpp>

#include <wrl.h>
#include <windows.h>
#include <WebView2.h>

namespace saucer
{
    using Microsoft::WRL::ComPtr;

    struct window::impl
    {
        utils::handle<HWND, DestroyWindow> hwnd;
    };

    struct webview::impl
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2> web_view;
    };
} // namespace saucer
