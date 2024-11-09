#pragma once

#include "scheme.hpp"

#include "webview.hpp"
#include "gtk.utils.hpp"

#include <webkit/webkit.h>

namespace saucer::scheme
{
    struct request::impl
    {
        utils::g_object_ptr<WebKitURISchemeRequest> request;
    };

    struct callback
    {
        application *app;

      public:
        launch policy;
        scheme::resolver resolver;
    };

    class handler
    {
        std::unordered_map<WebKitWebView *, callback> m_callbacks;

      public:
        void add_callback(WebKitWebView *, callback);
        void del_callback(WebKitWebView *);

      public:
        static void handle(WebKitURISchemeRequest *, handler *);
    };
} // namespace saucer::scheme
