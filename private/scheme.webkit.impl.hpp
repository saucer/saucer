#pragma once

#include "scheme.hpp"

#include <webkit/webkit.h>

namespace saucer
{
    struct request::impl
    {
        WebKitURISchemeRequest *request;
    };
} // namespace saucer
