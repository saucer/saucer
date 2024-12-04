#pragma once

#include "window.hpp"

#include "win32.utils.hpp"

#include <utility>
#include <optional>

#include <windows.h>

namespace saucer
{
    struct window::impl
    {
        utils::window_handle hwnd;

      public:
        UINT prev_state;

      public:
        bool decorated{true};
        bool transparent{false};

      public:
        utils::handle<HICON, DestroyIcon> icon;
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        WNDPROC o_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
