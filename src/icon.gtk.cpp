#include "icon.gtk.impl.hpp"

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

    bool icon::empty() const // NOLINT
    {
        return false;
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        auto bytes    = g_bytes_ptr{g_bytes_new(ico.data(), ico.size())};
        auto *texture = gdk_texture_new_from_bytes(bytes.get(), nullptr);

        if (!texture)
        {
            return std::nullopt;
        }

        return icon{{texture}};
    }

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto *texture = gdk_texture_new_from_filename(file.string().c_str(), nullptr);

        if (!texture)
        {
            return std::nullopt;
        }

        return icon{{texture}};
    }
} // namespace saucer
