#include "utils.win32.hpp"

#include <array>
#include <windows.h>

namespace saucer
{
    std::string last_error()
    {
        auto error = GetLastError();

        if (!error)
        {
            return "<No Error>";
        }

        std::array<WCHAR, 1024> buffer{};
        auto lang_id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        constexpr DWORD dw_flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        auto size = FormatMessageW(dw_flags, nullptr, error, lang_id, buffer.data(), buffer.size(), nullptr);

        return narrow(std::wstring{buffer.data(), size});
    }

    void set_dpi_awareness()
    {
        auto *shcore = LoadLibraryW(L"Shcore.dll");
        auto set_process_dpi_awareness = GetProcAddress(shcore, "SetProcessDpiAwareness");

        if (set_process_dpi_awareness)
        {
            reinterpret_cast<HRESULT(CALLBACK *)(DWORD)>(set_process_dpi_awareness)(2);
            return;
        }

        auto *user32 = LoadLibraryW(L"user32.dll");
        auto set_process_dpi_aware = GetProcAddress(user32, "SetProcessDPIAware");

        if (!set_process_dpi_aware)
        {
            return;
        }

        reinterpret_cast<bool(CALLBACK *)()>(set_process_dpi_aware)();
    }

    std::wstring widen(const std::string &narrow)
    {
        auto size = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), -1, nullptr, 0);

        if (!size)
        {
            return {};
        }

        std::wstring out(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), -1, out.data(), size);

        out.resize(size - 1);
        return out;
    }

    std::string narrow(const std::wstring &wide)
    {
        auto len = static_cast<int>(wide.length());
        auto size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, nullptr, 0, nullptr, nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), size, nullptr, nullptr);

        return out;
    }
} // namespace saucer