#pragma once

#include <saucer/url.hpp>

#include "ref_obj.hpp"

#include <glib.h>

namespace saucer
{
    struct url::impl
    {
        utils::ref_ptr<GUri, g_uri_ref, g_uri_unref> uri;
    };
} // namespace saucer
