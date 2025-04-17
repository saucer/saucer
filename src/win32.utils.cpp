#include "win32.utils.hpp"

#include <shellscalingapi.h>
#include <dwmapi.h>

namespace saucer
{
    template <auto Func, typename T, typename... Ts>
    auto call_as(T func, Ts &&...args)
    {
        using func_t = decltype(Func);
        return reinterpret_cast<func_t>(func)(std::forward<Ts>(args)...);
    }

    void utils::set_dpi_awareness()
    {
        auto user32 = module_handle{LoadLibraryW(L"user32.dll")};
        auto shcore = module_handle{LoadLibraryW(L"Shcore.dll")};

        if (auto *func = GetProcAddress(user32.get(), "SetProcessDpiAwarenessContext"); func)
        {
            call_as<SetProcessDpiAwarenessContext>(func, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }

        if (auto *func = GetProcAddress(shcore.get(), "SetProcessDpiAwareness"); func)
        {
            call_as<SetProcessDpiAwareness>(func, PROCESS_PER_MONITOR_DPI_AWARE);
            return;
        }

        auto *func = GetProcAddress(user32.get(), "SetProcessDPIAware");

        if (!func)
        {
            return;
        }

        call_as<SetProcessDPIAware>(func);
    }

    void utils::set_immersive_dark(HWND hwnd, bool enabled)
    {
        auto dwmapi = module_handle{LoadLibraryW(L"Dwmapi.dll")};
        auto *func  = GetProcAddress(dwmapi.get(), "DwmSetWindowAttribute");

        if (!func)
        {
            return;
        }

        static constexpr auto immersive_dark = 20;
        auto enable_immersive_dark           = static_cast<BOOL>(enabled);

        call_as<DwmSetWindowAttribute>(func, hwnd, immersive_dark, &enable_immersive_dark, sizeof(BOOL));
    }

    void utils::extend_frame(HWND hwnd, std::array<int, 4> area)
    {
        auto dwmapi = module_handle{LoadLibraryW(L"Dwmapi.dll")};
        auto *func  = GetProcAddress(dwmapi.get(), "DwmExtendFrameIntoClientArea");

        if (!func)
        {
            return;
        }

        const auto &[left, right, top, bottom] = area;
        const auto margins                     = MARGINS{left, right, top, bottom};

        call_as<DwmExtendFrameIntoClientArea>(func, hwnd, &margins);
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
