#pragma once

#include <saucer/uri.hpp>

#include <QUrl>

namespace saucer
{
    struct uri::impl
    {
        QUrl uri;
    };
} // namespace saucer
