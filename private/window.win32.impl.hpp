#pragma once

#include "window.hpp"

#include <thread>
#include <optional>

#include <windows.h>

namespace saucer
{
    template <typename T>
    using custom_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct window::impl
    {
        HWND hwnd;

      public:
        UINT last_state;
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static const UINT WM_SAFE_CALL;

      public:
        static inline HMODULE instance;
        static inline custom_ptr<HWND> handler;

      public:
        static std::atomic<std::size_t> instances;
        static inline std::optional<std::thread::id> creation_thread;

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    struct safe_message
    {
        using callback_t = std::move_only_function<void()>;

      public:
        callback_t callback;

      public:
        safe_message(callback_t callback);
    };
} // namespace saucer
