#pragma once

#include <saucer/navigation.hpp>

#include <webkit/webkit.h>

namespace saucer
{
    struct navigation::impl
    {
        WebKitNavigationPolicyDecision *decision;
        WebKitPolicyDecisionType type;
    };
} // namespace saucer
