#pragma once

#include <gtk.utils.hpp>
#include <saucer/modules/module.hpp>

#include <adwaita.h>
#include <webkit/webkit.h>

namespace saucer
{
    struct natives::window_impl
    {
        g_object_ptr<AdwApplicationWindow> window;
        GtkBox *content;
        AdwHeaderBar *header;
    };

    struct natives::webview_impl
    {
        g_object_ptr<WebKitWebView> web_view;
    };
} // namespace saucer
