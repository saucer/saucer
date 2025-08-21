#pragma once

#include <saucer/navigation.hpp>

#ifdef SAUCER_QT6
#include <variant>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineNavigationRequest>
#endif

namespace saucer
{
#ifdef SAUCER_QT6
    struct navigation::impl
    {
        std::variant<QWebEngineNewWindowRequest *, QWebEngineNavigationRequest *> request;
    };
#else
    struct navigation::impl
    {
    };
#endif
} // namespace saucer
