#include "utils.win32.hpp"

#include <stdexcept>
#include <string_view>

#include <windows.h>
#include <fmt/core.h>

namespace saucer
{
    class library
    {
        HMODULE m_module;

      public:
        library(std::wstring_view name) : m_module(LoadLibraryW(name.data())) {}

      public:
        ~library()
        {
            FreeLibrary(m_module);
        }

      public:
        [[nodiscard]] HMODULE value() const
        {
            return m_module;
        }
    };

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
        auto shcore = library{L"Shcore.dll"};

        if (auto *func = GetProcAddress(shcore.value(), "SetProcessDpiAwareness"); func)
        {
            reinterpret_cast<HRESULT(CALLBACK *)(DWORD)>(func)(2);
            return;
        }

        auto user32 = library{L"user32.dll"};
        auto *func  = GetProcAddress(user32.value(), "SetProcessDPIAware");

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
