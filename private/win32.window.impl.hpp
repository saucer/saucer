#pragma once

#include "window.hpp"

#include <optional>

#include <windows.h>

namespace saucer
{
    struct window::impl
    {
        HWND hwnd;

      public:
        UINT prev_state;
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        WNDPROC o_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
