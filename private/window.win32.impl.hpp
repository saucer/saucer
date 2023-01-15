#pragma once
#include "window.hpp"

#include <array>
#include <thread>
#include <future>
#include <Windows.h>

namespace saucer
{
    struct window::impl
    {
        using color_t = std::array<int, 4>;
        using bounds_t = std::array<int, 2>;

      public:
        HWND hwnd;
        bounds_t min_size, max_size;

      public:
        color_t background_color;
        std::function<void()> change_background;

      public:
        std::thread::id creation_thread;

      public:
        void set_background_color(const color_t &);

      public:
        [[nodiscard]] bool is_thread_safe() const;
        template <typename Func> auto post_safe(Func &&);

      public:
        static const UINT WM_SAFE_CALL;
        static std::atomic<std::size_t> open_windows;

      public:
        static void set_dpi_awareness();
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    struct message
    {
        virtual ~message() = default;
    };

    template <typename Return> class safe_message : public message
    {
        using callback_t = std::function<Return()>;

      private:
        callback_t m_func;
        std::promise<Return> &m_result;

      public:
        safe_message(callback_t &&func, std::promise<Return> &result) : m_func(func), m_result(result) {}

      public:
        ~safe_message() override
        {
            if constexpr (std::is_same_v<Return, void>)
            {
                m_func();
                m_result.set_value();
            }
            else
            {
                m_result.set_value(m_func());
            }
        }
    };

    template <typename Func> auto window::impl::post_safe(Func &&func)
    {
        using return_t = typename decltype(std::function(func))::result_type;

        std::promise<return_t> result;
        auto *message = new safe_message<return_t>(std::function(func), result);

        // ? the WndProc will delete the message after processing.
        PostMessage(hwnd, WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));

        return result.get_future().get();
    }
} // namespace saucer