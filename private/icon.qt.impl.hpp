#pragma once

#include "icon.hpp"

#include <QIcon>

namespace saucer
{
    struct icon::impl
    {
        QIcon icon;
    };
} // namespace saucer
