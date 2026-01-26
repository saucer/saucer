#include "win32.error.hpp"

namespace saucer
{
    std::string name_of(Gdiplus::Status value)
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

    error error::of<DWORD>::operator()(DWORD value)
    {
        return error::of<std::error_code>{}({static_cast<int>(value), std::system_category()});
    }

    error error::of<HRESULT>::operator()(HRESULT value)
    {
        return error::of<DWORD>{}(static_cast<DWORD>(value));
    }

    error error::of<Gdiplus::Status>::operator()(Gdiplus::Status value)
    {
        return {.code = static_cast<int>(value), .message = name_of(value), .kind = platform_domain()};
    }
} // namespace saucer
