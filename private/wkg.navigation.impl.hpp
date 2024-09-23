#pragma once

#include "navigation.hpp"
#include "gtk.utils.hpp"

#include <webkit/webkit.h>

namespace saucer
{
    struct navigation::impl
    {
        g_object_ptr<WebKitNavigationPolicyDecision> decision;

      public:
        WebKitPolicyDecisionType type;
    };
} // namespace saucer
