#pragma once

#include "scheme.hpp"

#include <webkit/webkit.h>

namespace saucer::scheme
{
    struct request::impl
    {
        WebKitURISchemeRequest *request;
    };

    struct state
    {
        handler handler;

      public:
        static void handle(WebKitURISchemeRequest *, state *);
    };
} // namespace saucer::scheme
