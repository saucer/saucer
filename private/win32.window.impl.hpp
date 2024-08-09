#pragma once

#include "window.hpp"

#include <thread>
#include <optional>

#include <windows.h>

namespace saucer
{
    struct window::impl
    {
        HWND hwnd;
        UINT prev_state;

      public:
        bool resizable;
        bool decorations;
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        static const UINT WM_SAFE_CALL;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static std::atomic<std::size_t> instances;
        static thread_local inline HMODULE instance;

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    class safe_message
    {
        using callback_t = std::move_only_function<void()>;

      private:
        callback_t m_callback;

      public:
        safe_message(callback_t callback);

      public:
        ~safe_message();
    };
} // namespace saucer
