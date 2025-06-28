#include "cocoa.icon.impl.hpp"

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
        return !m_impl->icon.get().isValid;
    }

    stash<> icon::data() const
    {
        const utils::autorelease_guard guard{};

        auto *const tiff = [m_impl->icon.get() TIFFRepresentation];
        auto *const rep  = [NSBitmapImageRep imageRepWithData:tiff];
        auto *const data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:[NSDictionary dictionary]];

        const auto *raw = reinterpret_cast<const std::uint8_t *>(data.bytes);
        return stash<>::from({raw, raw + data.length});
    }

    void icon::save(const fs::path &path) const
    {
        assert(path.extension() == ".png");

        const utils::autorelease_guard guard{};

        auto *const tiff = [m_impl->icon.get() TIFFRepresentation];
        auto *const rep  = [NSBitmapImageRep imageRepWithData:tiff];
        auto *const data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:[NSDictionary dictionary]];

        NSError *error{};
        [data writeToFile:[NSString stringWithUTF8String:path.c_str()] options:0 error:&error];
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        const utils::autorelease_guard guard{};

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
        const utils::autorelease_guard guard{};

        auto *const image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:file.c_str()]];

        if (!image)
        {
            return std::nullopt;
        }

        return icon{{image}};
    }
} // namespace saucer
