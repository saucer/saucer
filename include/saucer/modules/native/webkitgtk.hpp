#pragma once

#include <saucer/gtk.utils.hpp>
#include <saucer/utils/handle.hpp>
#include <saucer/modules/module.hpp>

#include <adwaita.h>
#include <webkit/webkit.h>

namespace saucer
{
    struct window::impl
    {
        utils::handle<GtkWindow *, gtk_window_destroy> window;
        utils::g_object_ptr<GtkCssProvider> style;

      public:
        GtkBox *content;
        AdwHeaderBar *header;
    };

    struct webview::impl
    {
        WebKitWebView *web_view;
    };
} // namespace saucer
