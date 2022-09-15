#pragma once
#include "module.hpp"

#include <wil/com.h>
#include <Windows.h>
#include <WebView2.h>

namespace saucer
{
    template <> struct module<backend_type::webview2>::webview_impl
    {
        wil::com_ptr<ICoreWebView2> webview;
        wil::com_ptr<ICoreWebView2Controller> webview_controller;
    };

    template <> struct module<backend_type::webview2>::window_impl
    {
        HWND hwnd;
    };
} // namespace saucer