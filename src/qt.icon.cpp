#include "qt.icon.impl.hpp"

#include <cassert>

#include <QPixmap>
#include <QBuffer>

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

    icon::icon() : m_impl(std::make_unique<impl>()) {}

    icon::icon(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    icon::icon(const icon &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    icon::icon(icon &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    icon::~icon() = default;

    icon &icon::operator=(const icon &other)
    {
        if (this != &other)
        {
            m_impl = std::make_unique<impl>(*other.m_impl);
        }

        return *this;
    }

    icon &icon::operator=(icon &&other) noexcept
    {
        if (this != &other)
        {
            m_impl = std::move(other.m_impl);
        }

        return *this;
    }

    bool icon::empty() const
    {
        return m_impl->icon.isNull();
    }

    stash<> icon::data() const
    {
        auto pixmap = m_impl->pixmap();

        if (!pixmap)
        {
            return stash<>::empty();
        }

        QByteArray bytes;

        QBuffer buffer{&bytes};
        pixmap->save(&buffer, "PNG");

        return stash<>::from({bytes.begin(), bytes.end()});
    }

    void icon::save(const fs::path &path) const
    {
        assert(path.extension() == ".png");

        auto pixmap = m_impl->pixmap();

        if (!pixmap)
        {
            return;
        }

        pixmap->save(path.c_str());
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        QPixmap rtn{};

        if (!rtn.loadFromData(ico.data(), ico.size()))
        {
            return std::nullopt;
        }

        return icon{{rtn}};
    }

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto rtn = QIcon{QString::fromStdString(file.string())};

        if (rtn.isNull())
        {
            return std::nullopt;
        }

        return icon{{rtn}};
    }
} // namespace saucer
