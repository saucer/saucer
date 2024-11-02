#pragma once

#include "navigation.hpp"

#include <variant>

#include <WebView2.h>

namespace saucer
{
    struct navigation::impl
    {
        std::variant<ICoreWebView2NavigationStartingEventArgs *, ICoreWebView2NewWindowRequestedEventArgs *> request;
    };
} // namespace saucer
