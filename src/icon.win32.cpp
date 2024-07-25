#include "icon.win32.impl.hpp"

#include <shlwapi.h>

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
        return !m_impl->bitmap || m_impl->bitmap->GetLastStatus() != Gdiplus::Status::Ok;
    }

    std::optional<icon> icon::from(const stash<> &ico)
    {
        ComPtr<IStream> data = SHCreateMemStream(ico.data(), static_cast<DWORD>(ico.size()));
        auto *bitmap         = Gdiplus::Bitmap::FromStream(data.Get());

        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Status::Ok)
        {
            delete bitmap;
            return std::nullopt;
        }

        return icon{{std::shared_ptr<Gdiplus::Bitmap>{bitmap}}};
    }

    std::optional<icon> icon::from(const fs::path &file)
    {
        auto *bitmap = Gdiplus::Bitmap::FromFile(file.wstring().c_str());

        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Status::Ok)
        {
            delete bitmap;
            return std::nullopt;
        }

        return icon{{std::shared_ptr<Gdiplus::Bitmap>{bitmap}}};
    }
} // namespace saucer
