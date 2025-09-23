#pragma once

#include <saucer/modules/module.hpp>

#include <saucer/app.hpp>
#include <saucer/window.hpp>
#include <saucer/webview.hpp>

#include <windows.h>
#include <gdiplus.h>
#include <wininet.h>

#include <WebView2.h>

namespace saucer
{
    template <>
    struct stable_natives<application>
    {
        WNDCLASSW wnd_class;
    };

    template <>
    struct stable_natives<window>
    {
        HWND hwnd;
    };

    template <>
    struct stable_natives<webview>
    {
        ICoreWebView2_2 *webview;
        ICoreWebView2Controller *controller;
    };

    template <>
    struct stable_natives<permission::request>
    {
        ICoreWebView2PermissionRequestedEventArgs *request;
    };

    template <>
    struct stable_natives<url>
    {
        std::wstring *url;
        URL_COMPONENTSW *components;
    };

    template <>
    struct stable_natives<icon>
    {
        Gdiplus::Bitmap *icon;
    };
} // namespace saucer
