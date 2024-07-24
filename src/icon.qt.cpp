#include "icon.qt.impl.hpp"

#include <QPixmap>

namespace saucer
{
    icon::icon(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    icon::icon(const icon &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    icon::icon(icon &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    icon::~icon() = default;

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto q_icon = QIcon{QString::fromStdString(file.string())};

        if (q_icon.isMask())
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
