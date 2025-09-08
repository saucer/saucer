#pragma once

#include <saucer/error/error.hpp>

#include "handle.hpp"

#include <string>
#include <cstdint>

#include <span>
#include <array>
#include <vector>

#include <windows.h>

#include <winrt/windows.ui.composition.desktop.h>
#include <winrt/windows.system.h>

namespace saucer::utils
{
    using dispatch_controller = winrt::Windows::System::DispatcherQueueController;
    using compositor          = winrt::Windows::UI::Composition::Compositor;
    using brush               = winrt::Windows::UI::Composition::CompositionColorBrush;
    using window_target       = winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget;

    using string_handle  = utils::handle<LPWSTR, CoTaskMemFree>;
    using module_handle  = utils::handle<HMODULE, FreeLibrary>;
    using window_handle  = utils::handle<HWND, DestroyWindow>;
    using atom_handle    = utils::handle<ATOM, GlobalDeleteAtom>;
    using process_handle = utils::handle<HANDLE, CloseHandle>;

    // One of the earliest Windows 11 Build Numbers:
    // https://en.wikipedia.org/wiki/Windows_11_version_history
    static constexpr auto windows_11_build = 22000;

    class wnd_proc_hook
    {
        HWND m_hwnd{nullptr};
        WNDPROC m_original{nullptr};

      public:
        wnd_proc_hook();
        wnd_proc_hook(HWND, WNDPROC);

      public:
        wnd_proc_hook(wnd_proc_hook &&) noexcept;
        wnd_proc_hook &operator=(wnd_proc_hook &&) noexcept;

      public:
        ~wnd_proc_hook();

      public:
        [[nodiscard]] WNDPROC original() const;
    };

    void set_dpi_awareness();
    void set_immersive_dark(HWND, bool);
    void extend_frame(HWND, std::array<int, 4>);

    [[nodiscard]] OSVERSIONINFOEXW version();

    [[nodiscard]] std::wstring widen(std::string_view);
    [[nodiscard]] std::string narrow(std::wstring_view);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *);

    [[nodiscard]] result<dispatch_controller> create_dispatch_controller();
    [[nodiscard]] result<window_target> create_window_target(const compositor &, HWND);

    [[nodiscard]] result<std::wstring> hash(std::span<std::uint8_t>);
} // namespace saucer::utils
