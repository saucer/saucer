#pragma once

#include "handle.hpp"

#include <string>
#include <cstdint>

#include <array>
#include <vector>

#include <windows.h>

#include <winrt/windows.ui.composition.desktop.h>
#include <winrt/windows.system.h>

namespace saucer::utils
{
    namespace system      = winrt::Windows::System;
    namespace composition = winrt::Windows::UI::Composition;

    using dispatch_controller = system::DispatcherQueueController;
    using compositor          = composition::Compositor;
    using window_target       = composition::Desktop::DesktopWindowTarget;

    using string_handle = utils::handle<LPWSTR, CoTaskMemFree>;
    using module_handle = utils::handle<HMODULE, FreeLibrary>;
    using window_handle = utils::handle<HWND, DestroyWindow>;
    using atom_handle   = utils::handle<ATOM, GlobalDeleteAtom>;

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

    [[nodiscard]] std::wstring widen(const std::string &);
    [[nodiscard]] std::string narrow(const std::wstring &);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *);

    [[nodiscard]] std::optional<dispatch_controller> create_dispatch_controller();
    [[nodiscard]] std::optional<window_target> create_window_target(const compositor &, HWND);
} // namespace saucer::utils
