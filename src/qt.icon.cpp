#include "qt.icon.impl.hpp"

#include "error.impl.hpp"

#include <cassert>

#include <QPixmap>
#include <QBuffer>

namespace saucer
{
    icon::icon() : m_impl(std::make_unique<impl>()) {}

    icon::icon(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    icon::icon(const icon &other) : icon(*other.m_impl) {}

    icon::icon(icon &&other) noexcept : icon()
    {
        swap(*this, other);
    }

    icon::~icon() = default;

    icon &icon::operator=(icon other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(icon &first, icon &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    bool icon::empty() const
    {
        return m_impl->icon.isNull();
    }

    stash icon::data() const
    {
        auto pixmap = m_impl->pixmap();

        if (!pixmap)
        {
            return stash::empty();
        }

        QByteArray bytes;

        QBuffer buffer{&bytes};
        pixmap->save(&buffer, "PNG");

        return stash::from({bytes.begin(), bytes.end()});
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

    result<icon> icon::from(const stash &ico)
    {
        QPixmap rtn{};

        if (!rtn.loadFromData(ico.data(), ico.size()))
        {
            return err(std::error_code{});
        }

        return icon{{rtn}};
    }

    result<icon> icon::from(const fs::path &file)
    {
        auto rtn = QIcon{QString::fromStdString(file.string())};

        if (rtn.isNull())
        {
            return err(std::error_code{});
        }

        return icon{{rtn}};
    }
} // namespace saucer
