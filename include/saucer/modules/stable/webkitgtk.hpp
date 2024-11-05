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
    struct stable_natives<application>
    {
        AdwApplication *application;
    };

    template <>
    struct stable_natives<window>
    {
        GtkWindow *window;
    };

    template <>
    struct stable_natives<webview>
    {
        WebKitWebView *webview;
    };
} // namespace saucer
