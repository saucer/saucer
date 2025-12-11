#include "win32.utils.hpp"
#include "win32.error.hpp"

#include <shlwapi.h>
#include <shellscalingapi.h>

#include <dwmapi.h>
#include <dispatcherqueue.h>
#include <windows.ui.composition.interop.h>

#include <ranges>

namespace saucer::utils
{
    wnd_proc_hook::wnd_proc_hook() = default;

    wnd_proc_hook::wnd_proc_hook(HWND hwnd, WNDPROC proc) : m_hwnd(hwnd)
    {
        auto ptr   = reinterpret_cast<LONG_PTR>(proc);
        m_original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, ptr));
    }

    wnd_proc_hook::wnd_proc_hook(wnd_proc_hook &&other) noexcept
        : m_hwnd(std::exchange(other.m_hwnd, nullptr)), m_original(std::exchange(other.m_original, nullptr))
    {
    }

    wnd_proc_hook &wnd_proc_hook::operator=(wnd_proc_hook &&other) noexcept
    {
        if (this != &other)
        {
            m_hwnd     = std::exchange(other.m_hwnd, nullptr);
            m_original = std::exchange(other.m_original, nullptr);
        }

        return *this;
    }

    wnd_proc_hook::~wnd_proc_hook()
    {
        if (!m_hwnd)
        {
            return;
        }

        SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_original));
    }

    WNDPROC wnd_proc_hook::original() const
    {
        return m_original;
    }
} // namespace saucer::utils

namespace saucer
{
    template <typename Func, typename T, typename... Ts>
    auto call_as(T func, Ts &&...args)
    {
        return reinterpret_cast<Func>(func)(std::forward<Ts>(args)...);
    }

    void utils::set_dpi_awareness()
    {
        auto user32 = module_handle{LoadLibraryW(L"user32.dll")};
        auto shcore = module_handle{LoadLibraryW(L"Shcore.dll")};

        if (auto *func = GetProcAddress(user32.get(), "SetProcessDpiAwarenessContext"); func)
        {
            call_as<decltype(SetProcessDpiAwarenessContext)>(func, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }

        if (auto *func = GetProcAddress(shcore.get(), "SetProcessDpiAwareness"); func)
        {
            call_as<decltype(SetProcessDpiAwareness)>(func, PROCESS_PER_MONITOR_DPI_AWARE);
            return;
        }

        auto *func = GetProcAddress(user32.get(), "SetProcessDPIAware");

        if (!func)
        {
            return;
        }

        call_as<decltype(SetProcessDPIAware)>(func);
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

        call_as<decltype(DwmSetWindowAttribute)>(func, hwnd, immersive_dark, &enable_immersive_dark, sizeof(BOOL));
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

        call_as<decltype(DwmExtendFrameIntoClientArea)>(func, hwnd, &margins);
    }

    OSVERSIONINFOEXW utils::version()
    {
        auto ntdll = module_handle{LoadLibraryW(L"ntdll.dll")};
        auto *func = GetProcAddress(ntdll.get(), "RtlGetVersion");

        if (!func)
        {
            return {};
        }

        OSVERSIONINFOEXW rtn{};
        std::invoke(reinterpret_cast<NTSTATUS(WINAPI *)(LPOSVERSIONINFOEXW)>(func), &rtn);

        return rtn;
    }

    std::wstring utils::widen(std::string_view narrow)
    {
        auto size = MultiByteToWideChar(CP_UTF8, 0, narrow.data(), static_cast<int>(narrow.size()), nullptr, 0);

        if (!size)
        {
            return {};
        }

        std::wstring out(size, '\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, narrow.data(), static_cast<int>(narrow.size()), out.data(), size);

        return out;
    }

    std::string utils::narrow(std::wstring_view wide)
    {
        auto size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr);

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

    result<utils::dispatch_controller> utils::create_dispatch_controller()
    {
        using ABI::Windows::System::IDispatcherQueueController;

        auto rtn         = dispatch_controller{nullptr};
        auto *controller = reinterpret_cast<IDispatcherQueueController **>(winrt::put_abi(rtn));

        DispatcherQueueOptions options{
            .dwSize        = sizeof(DispatcherQueueOptions),
            .threadType    = DQTYPE_THREAD_CURRENT,
            .apartmentType = DQTAT_COM_STA,
        };

        if (auto status = CreateDispatcherQueueController(options, controller); !SUCCEEDED(status))
        {
            return err(status);
        }

        return rtn;
    }

    result<utils::window_target> utils::create_window_target(const compositor &comp, HWND hwnd)
    {
        using ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop;
        using ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget;

        auto interop = comp.as<ICompositorDesktopInterop>();

        auto rtn     = window_target{nullptr};
        auto *target = reinterpret_cast<IDesktopWindowTarget **>(winrt::put_abi(rtn));

        if (auto status = interop->CreateDesktopWindowTarget(hwnd, false, target); !SUCCEEDED(status))
        {
            return err(status);
        }

        return rtn;
    }

    result<std::wstring> utils::hash(std::span<std::uint8_t> value)
    {
        static constexpr auto size = 32;

        BYTE buffer[size]{};

        if (auto status = HashData(value.data(), value.size(), buffer, size); !SUCCEEDED(status))
        {
            return err(status);
        }

        return buffer                                                                  //
               | std::views::transform([](auto x) { return std::format(L"{:x}", x); }) //
               | std::views::join                                                      //
               | std::ranges::to<std::wstring>();
    }
} // namespace saucer
