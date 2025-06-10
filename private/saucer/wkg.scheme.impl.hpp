#pragma once

#include "scheme.hpp"

#include "gtk.utils.hpp"

#include <webkit/webkit.h>

namespace saucer::scheme
{
    struct request::impl
    {
        utils::g_object_ptr<WebKitURISchemeRequest> request;
    };

    class handler
    {
        std::unordered_map<WebKitWebView *, scheme::resolver> m_callbacks;

      public:
        void add_callback(WebKitWebView *, scheme::resolver);
        void del_callback(WebKitWebView *);

      public:
        static void handle(WebKitURISchemeRequest *, handler *);
    };
} // namespace saucer::scheme
