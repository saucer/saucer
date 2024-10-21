#pragma once

#include "scheme.hpp"

#include <webkit/webkit.h>

namespace saucer::scheme
{
    struct request::impl
    {
        WebKitURISchemeRequest *request;
    };

    class scheme_handler
    {
        std::unordered_map<WebKitWebView *, handler> m_handlers;

      public:
        void add_handler(WebKitWebView *, handler);
        void remove_handler(WebKitWebView *);

      public:
        static void handle(WebKitURISchemeRequest *, scheme_handler *);
    };
} // namespace saucer::scheme
