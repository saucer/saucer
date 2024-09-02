#include "cocoa.icon.impl.hpp"

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
        return !m_impl->icon.isValid;
    }

    stash<> icon::data() const
    {
        auto *const tiff = [m_impl->icon TIFFRepresentation];
        auto *const rep  = [NSBitmapImageRep imageRepWithData:tiff];
        auto *const data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:[NSDictionary dictionary]];

        const auto *raw = reinterpret_cast<const std::uint8_t *>(data.bytes);
        return stash<>::from({raw, raw + data.length});
    }

    void icon::save(const fs::path &path) const
    {
        assert(path.extension() == ".png");

        auto *const tiff = [m_impl->icon TIFFRepresentation];
        auto *const rep  = [NSBitmapImageRep imageRepWithData:tiff];
        auto *const data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:[NSDictionary dictionary]];

        NSError *error{};
        [data writeToFile:[NSString stringWithUTF8String:path.c_str()] options:0 error:&error];
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        auto *const data  = [NSData dataWithBytes:ico.data() length:ico.size()];
        auto *const image = [[NSImage alloc] initWithData:data];

        if (!image)
        {
            return std::nullopt;
        }

        return icon{{image}};
    }

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto *const image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:file.c_str()]];

        if (!image)
        {
            return std::nullopt;
        }

        return icon{{image}};
    }
} // namespace saucer
