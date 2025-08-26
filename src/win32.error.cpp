#include "win32.error.hpp"

namespace saucer
{
    error::of<DWORD>::of(DWORD value) : error::of<std::error_code>({static_cast<int>(value), std::system_category()}) {}

    error::of<HRESULT>::of(HRESULT value) : error::of<DWORD>(static_cast<DWORD>(value)) {}

    error::of<Gdiplus::Status>::of(Gdiplus::Status value) : value(value) {}

    int error::of<Gdiplus::Status>::code() const
    {
        return std::to_underlying(value);
    }

    error::category error::of<Gdiplus::Status>::type() const
    {
        return category::platform;
    }

    std::string error::of<Gdiplus::Status>::message() const
    {
        switch (value)
        {
        case Gdiplus::Ok:
            return "Ok";

        case Gdiplus::GenericError:
            return "GenericError";

        case Gdiplus::InvalidParameter:
            return "InvalidParameter";

        case Gdiplus::OutOfMemory:
            return "OutOfMemory";

        case Gdiplus::ObjectBusy:
            return "ObjectBusy";

        case Gdiplus::InsufficientBuffer:
            return "InsufficientBuffer";

        case Gdiplus::NotImplemented:
            return "NotImplemented";

        case Gdiplus::Win32Error:
            return "Win32Error";

        case Gdiplus::WrongState:
            return "WrongState";

        case Gdiplus::Aborted:
            return "Aborted";

        case Gdiplus::FileNotFound:
            return "FileNotFound";

        case Gdiplus::ValueOverflow:
            return "ValueOverflow";

        case Gdiplus::AccessDenied:
            return "AccessDenied";

        case Gdiplus::UnknownImageFormat:
            return "UnknownImageFormat";

        case Gdiplus::FontFamilyNotFound:
            return "FontFamilyNotFound";

        case Gdiplus::FontStyleNotFound:
            return "FontStyleNotFound";

        case Gdiplus::NotTrueTypeFont:
            return "NotTrueTypeFont";

        case Gdiplus::UnsupportedGdiplusVersion:
            return "UnsupportedGdiplusVersion";

        case Gdiplus::GdiplusNotInitialized:
            return "GdiplusNotInitialized";

        case Gdiplus::PropertyNotFound:
            return "PropertyNotFound";

        case Gdiplus::PropertyNotSupported:
            return "PropertyNotSupported";
        }

        std::unreachable();
    }
} // namespace saucer
