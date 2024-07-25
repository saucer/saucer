#include "icon.qt.impl.hpp"

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

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto q_icon = QIcon{QString::fromStdString(file.string())};

        if (q_icon.isNull())
        {
            return std::nullopt;
        }

        return icon{{q_icon}};
    }

    std::optional<icon> icon::from(const stash<const std::uint8_t> &ico)
    {
        QPixmap rtn{};

        if (!rtn.loadFromData(ico.data(), ico.size()))
        {
            return std::nullopt;
        }

        return icon{{rtn}};
    }
} // namespace saucer
