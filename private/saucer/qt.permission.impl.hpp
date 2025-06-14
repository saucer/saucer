#pragma once

#include "permission.hpp"

#include <QWebEnginePermission>

namespace saucer::permission
{
    struct request::impl
    {
        QWebEnginePermission request;
    };
} // namespace saucer::permission
