#pragma once

#include "navigation.hpp"

#include <variant>

#include <QWebEngineNewWindowRequest>
#include <QWebEngineNavigationRequest>

namespace saucer
{
    struct navigation::impl
    {
        std::variant<QWebEngineNewWindowRequest *, QWebEngineNavigationRequest *> request;
    };
} // namespace saucer
