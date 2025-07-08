#pragma once

#include "permission.hpp"

#ifdef SAUCER_QT6
#include <QWebEnginePermission>
#endif

namespace saucer::permission
{
    struct request::impl
    {
#ifdef SAUCER_QT6
        QWebEnginePermission request;

      public:
        QWebEnginePermission::PermissionType type;
        QUrl origin;
#endif
    };
} // namespace saucer::permission
