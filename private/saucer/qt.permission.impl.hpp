#pragma once

#include <saucer/permission.hpp>

#include <QWebEnginePermission>

namespace saucer::permission
{
    struct request::impl
    {
        QWebEnginePermission request;

      public:
        QWebEnginePermission::PermissionType type;
        QUrl origin;
    };
} // namespace saucer::permission
