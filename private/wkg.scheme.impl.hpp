#pragma once

#include "scheme.hpp"

#include <webkit/webkit.h>

namespace saucer
{
    struct request::impl
    {
        WebKitURISchemeRequest *request;
    };

    struct scheme_state
    {
        scheme_handler handler;
        static void handle(WebKitURISchemeRequest *, scheme_state *);
    };
} // namespace saucer
