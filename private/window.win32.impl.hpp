#pragma once

#include "window.hpp"

#include <thread>
#include <future>

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
        color background;
        std::function<void()> change_background;

      public:
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        [[nodiscard]] bool is_thread_safe() const;
        [[nodiscard]] std::pair<int, int> window_offset() const;

      public:
        static const UINT WM_SAFE_CALL;
        static std::atomic<std::size_t> instances;

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

      public:
        template <typename Func>
        auto post_safe(Func &&);
    };

    struct message
    {
        virtual ~message() = default;
    };

    template <typename T>
    class safe_message : public message
    {
        using callback_t = std::function<T()>;

      private:
        callback_t m_func;
        std::promise<T> *m_result;

      public:
        safe_message(callback_t &&func, std::promise<T> *result) : m_func(std::move(func)), m_result(result) {}

      public:
        ~safe_message() override
        {
            if constexpr (!std::is_void_v<T>)
            {
                m_result->set_value(std::invoke(m_func));
            }
            else
            {
                std::invoke(m_func);
                m_result->set_value();
            }
        }
    };

    template <typename Func>
    auto window::impl::post_safe(Func &&func)
    {
        using return_t = typename decltype(std::function(func))::result_type;

        std::promise<return_t> result;
        auto *message = new safe_message<return_t>(std::forward<Func>(func), &result);

        // ? the WndProc will delete the message after processing.
        PostMessage(hwnd, WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));

        return result.get_future().get();
    }
} // namespace saucer
