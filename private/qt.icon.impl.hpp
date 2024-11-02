#pragma once

#include "icon.hpp"

#include <QIcon>

namespace saucer
{
    struct icon::impl
    {
        QIcon icon;

      public:
        [[nodiscard]] std::optional<QPixmap> pixmap() const;
    };
} // namespace saucer
