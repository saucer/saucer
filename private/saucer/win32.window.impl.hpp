#pragma once

#include "window.impl.hpp"

#include "win32.utils.hpp"

#include <optional>

#include <windows.h>

namespace saucer
{
    struct window_flags
    {
        bool resizable{true};
        bool click_through{false};

      public:
        bool fullscreen{false};
        window::decoration decorations{window::decoration::full};

      public:
        void apply(HWND) const;

      public:
        static constexpr auto standard = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    };

    enum class mode : std::uint8_t
    {
        add,
        sub
    };

    struct window::impl::native
    {
        utils::window_handle hwnd;

      public:
        UINT dpi;
        UINT prev_state;

      public:
        std::optional<decoration> prev_decoration;
        std::optional<WINDOWPLACEMENT> prev_placement;

      public:
        utils::brush background{nullptr};
        utils::window_target window_target{nullptr};

      public:
        window_flags flags;

      public:
        utils::wnd_proc_hook hook;
        utils::handle<HICON, DestroyIcon> icon;
        std::optional<saucer::size> max_size, min_size;

      public:
        template <mode>
        [[nodiscard]] saucer::size scale(saucer::size) const;

        template <mode>
        [[nodiscard]] saucer::size offset(saucer::size) const;

      public:
        static inline std::size_t windows_build;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
