#pragma once
#include <wil/com.h>
#include <Windows.h>
#include <WebView2.h>
#include <saucer/module/module.hpp>

namespace saucer
{
    template <> struct module::webview_impl<backend_type::webview2>
    {
        wil::com_ptr<ICoreWebView2> webview;
        wil::com_ptr<ICoreWebView2Controller> webview_controller;
    };

    template <> struct module::window_impl<backend_type::webview2>
    {
        HWND hwnd;
    };
} // namespace saucer