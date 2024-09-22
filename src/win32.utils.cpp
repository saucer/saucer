#include "win32.utils.hpp"

#include <fmt/core.h>
#include <string_view>

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

    void utils::set_immersive_dark(HWND hwnd, bool enabled)
    {
        auto dwmapi = library{L"Dwmapi.dll"};
        auto *func  = GetProcAddress(dwmapi.value(), "DwmSetWindowAttribute");

        if (!func)
        {
            return;
        }

        static constexpr auto immersive_dark = 20;

        auto *set_attribute        = reinterpret_cast<HRESULT (*)(HWND, DWORD, LPCVOID, DWORD)>(func);
        auto enable_immersive_dark = static_cast<BOOL>(enabled);

        set_attribute(hwnd, immersive_dark, &enable_immersive_dark, sizeof(BOOL));
    }

    WNDPROC utils::overwrite_wndproc(HWND hwnd, WNDPROC wndproc)
    {
        auto ptr = reinterpret_cast<LONG_PTR>(wndproc);
        return reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, ptr));
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

    std::vector<std::uint8_t> utils::read(IStream *stream)
    {
        STATSTG stats;
        stream->Stat(&stats, STATFLAG_DEFAULT);

        std::vector<std::uint8_t> data;
        data.resize(stats.cbSize.QuadPart);

        ULONG read{};
        stream->Read(data.data(), static_cast<ULONG>(data.size()), &read);

        return data;
    }
} // namespace saucer
