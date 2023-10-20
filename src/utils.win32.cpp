#include "utils.win32.hpp"

#include <array>
#include <windows.h>

namespace saucer
{
    void utils::set_dpi_awareness()
    {
        auto *shcore = LoadLibraryW(L"Shcore.dll");
        auto *func   = GetProcAddress(shcore, "SetProcessDpiAwareness");

        if (func)
        {
            reinterpret_cast<HRESULT(CALLBACK *)(DWORD)>(func)(2);
            return;
        }

        auto *user32 = LoadLibraryW(L"user32.dll");
        func         = GetProcAddress(user32, "SetProcessDPIAware");

        if (!func)
        {
            return;
        }

        reinterpret_cast<bool(CALLBACK *)()>(func)();
    }

    std::string utils::error()
    {
        auto error = GetLastError();

        if (error == 0)
        {
            return {};
        }

        static constexpr auto lang_flags    = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        static constexpr DWORD format_flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        WCHAR buffer[1024] = {};
        auto size          = FormatMessageW(format_flags, nullptr, error, lang_flags, buffer, 1024, nullptr);

        return narrow(std::wstring{buffer, size});
    }

    std::wstring utils::widen(const std::string &narrow)
    {
        auto size = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), narrow.size(), nullptr, 0);

        if (size <= 0)
        {
            return {};
        }

        std::wstring out(size, '\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, narrow.c_str(), narrow.size(), out.data(), out.size());

        return out;
    }

    std::string utils::narrow(const std::wstring &wide)
    {
        auto size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wide.size(), nullptr, 0, nullptr, nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wide.size(), out.data(), size, nullptr, nullptr);

        return out;
    }
} // namespace saucer
