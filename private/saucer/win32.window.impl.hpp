#pragma once

#include "window.impl.hpp"

#include "win32.utils.hpp"

#include <optional>

#include <windows.h>

namespace saucer
{
    struct window::impl::native
    {
        utils::window_handle hwnd;

      public:
        UINT prev_state;
        std::optional<decoration> prev_decoration;

      public:
        utils::window_target window_target{nullptr};
        utils::composition::CompositionColorBrush background{nullptr};

      public:
        long styles{};
        bool titlebar{true};

      public:
        utils::wnd_proc_hook hook;
        utils::handle<HICON, DestroyIcon> icon;
        std::optional<saucer::size> max_size, min_size;

      public:
        static void set_style(HWND, long);
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
