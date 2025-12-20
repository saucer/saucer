#include "gtk.icon.impl.hpp"

#include "gtk.error.hpp"

#include <cassert>

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
        return !m_impl->texture;
    }

    stash icon::data() const
    {
        if (!m_impl->texture)
        {
            return stash::empty();
        }

        const auto bytes = utils::g_bytes_ptr{gdk_texture_save_to_png_bytes(m_impl->texture.get())};

        gsize size{};
        const auto *data = reinterpret_cast<const std::uint8_t *>(g_bytes_get_data(bytes.get(), &size));

        return stash::from({data, data + size});
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

    result<icon> icon::from(const stash &ico)
    {
        auto error          = utils::g_error_ptr{};
        const auto bytes    = utils::g_bytes_ptr{g_bytes_new(ico.data(), ico.size())};
        auto *const texture = gdk_texture_new_from_bytes(bytes.get(), &error.reset());

        if (!texture)
        {
            return err(std::move(error));
        }

        return icon{{texture}};
    }

    result<icon> icon::from(const fs::path &file)
    {
        auto error          = utils::g_error_ptr{};
        auto *const texture = gdk_texture_new_from_filename(file.string().c_str(), &error.reset());

        if (!texture)
        {
            return err(std::move(error));
        }

        return icon{{texture}};
    }
} // namespace saucer
