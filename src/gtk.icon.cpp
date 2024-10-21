#include "gtk.icon.impl.hpp"

#include <cassert>

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
        return !m_impl->texture;
    }

    stash<> icon::data() const
    {
        if (!m_impl->texture)
        {
            return stash<>::empty();
        }

        const auto bytes = utils::g_bytes_ptr{gdk_texture_save_to_png_bytes(m_impl->texture.get())};

        gsize size{};
        const auto *data = reinterpret_cast<const std::uint8_t *>(g_bytes_get_data(bytes.get(), &size));

        return stash<>::from({data, data + size});
    }

    void icon::save(const fs::path &path) const
    {
        assert(path.extension() == ".png");

        if (!m_impl->texture)
        {
            return;
        }

        gdk_texture_save_to_png(m_impl->texture.get(), path.c_str());
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        const auto bytes    = utils::g_bytes_ptr{g_bytes_new(ico.data(), ico.size())};
        auto *const texture = gdk_texture_new_from_bytes(bytes.get(), nullptr);

        if (!texture)
        {
            return std::nullopt;
        }

        return icon{{texture}};
    }

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto *const texture = gdk_texture_new_from_filename(file.string().c_str(), nullptr);

        if (!texture)
        {
            return std::nullopt;
        }

        return icon{{texture}};
    }
} // namespace saucer
