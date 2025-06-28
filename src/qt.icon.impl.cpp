#include "qt.icon.impl.hpp"

#include <QPixmap>

namespace saucer
{
    std::optional<QPixmap> icon::impl::pixmap() const
    {
        auto sizes = icon.availableSizes();

        if (sizes.empty())
        {
            return std::nullopt;
        }

        auto compare = [](const QSize &a, const QSize &b)
        {
            return (a.height() + a.width()) > (b.height() + b.width());
        };
        std::sort(sizes.begin(), sizes.end(), compare);

        return icon.pixmap(sizes.first());
    }
} // namespace saucer
