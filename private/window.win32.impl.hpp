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

      public:
        std::thread::id creation_thread;

      public:
        UINT last_state;
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        static const UINT WM_SAFE_CALL;
        static std::atomic<std::size_t> instances;

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
