#pragma once

#include "navigation.hpp"

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
#include <variant>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineNavigationRequest>
#endif

namespace saucer
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
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
