#pragma once

#include <saucer/modules/module.hpp>

#include <saucer/app.hpp>
#include <saucer/window.hpp>
#include <saucer/webview.hpp>

#include <adwaita.h>
#include <webkit/webkit.h>

namespace saucer
{
    template <>
    struct stable<application>
    {
        AdwApplication *application;
    };

    template <>
    struct stable<window>
    {
        GtkWindow *window;
    };

    template <>
    struct stable<webview>
    {
        WebKitWebView *webview;
    };
} // namespace saucer
