#pragma once

#include <saucer/url.hpp>

#include <QUrl>

namespace saucer
{
    struct url::impl
    {
        QUrl url;
    };
} // namespace saucer
