#include "qt.icon.impl.hpp"

#include <QPixmap>

namespace saucer
{
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

    void icon::save(const fs::path &path) const
    {
        auto sizes = m_impl->icon.availableSizes();

        if (sizes.empty())
        {
            return;
        }

        auto compare = [](const QSize &a, const QSize &b)
        {
            return (a.height() + a.width()) > (b.height() + b.width());
        };
        std::ranges::sort(sizes, compare);

        m_impl->icon.pixmap(sizes.first()).save(path.c_str());
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
