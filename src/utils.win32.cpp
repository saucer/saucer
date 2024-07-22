#include "utils.win32.hpp"

#include <windows.h>

#include <stdexcept>
#include <fmt/core.h>

namespace saucer
{
    void utils::throw_error(const std::string &msg)
    {
        static constexpr auto lang    = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        static constexpr DWORD format = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        std::wstring error(1024, '\0');
        FormatMessageW(format, nullptr, GetLastError(), lang, error.data(), static_cast<DWORD>(error.size()), nullptr);

        throw std::runtime_error(fmt::format("An error occurred: {} (LastError: \"{}\")", msg, utils::narrow(error)));
    }

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

    std::wstring utils::widen(const std::string &narrow)
    {
        auto narrow_size = static_cast<int>(narrow.size());
        auto size        = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), narrow_size, nullptr, 0);

        if (!size)
        {
            return {};
        }

        std::wstring out(size, '\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, narrow.c_str(), narrow_size, out.data(), size);

        return out;
    }

    std::string utils::narrow(const std::wstring &wide)
    {
        auto wide_size = static_cast<int>(wide.size());
        auto size      = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wide_size, nullptr, 0, nullptr, nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wide_size, out.data(), size, nullptr, nullptr);

        return out;
    }
} // namespace saucer
