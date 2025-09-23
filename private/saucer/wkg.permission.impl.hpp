#pragma once

#include <saucer/permission.hpp>

#include "gtk.utils.hpp"

#include <webkit/webkit.h>

namespace saucer::permission
{
    struct request::impl
    {
        utils::g_object_ptr<WebKitPermissionRequest> request;

      public:
        saucer::url url;
        permission::type type;
    };
} // namespace saucer::permission
