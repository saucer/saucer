#pragma once
#include "window.hpp"
#include "utils/color.hpp"

#include <thread>
#include <future>
#include <Windows.h>

namespace saucer
{
    struct window::impl
    {
        HWND hwnd;

      public:
        std::thread::id creation_thread;

      public:
        color background_color;
        std::function<void()> change_background;

      public:
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        void set_background_color(const color &);

      public:
        template <typename Func>
        auto post_safe(Func &&);

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        static const UINT WM_SAFE_CALL;
        static std::atomic<std::size_t> open_windows;

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    struct message
    {
        virtual ~message() = default;
    };

    template <typename Return>
    class safe_message : public message
    {
        using callback_t = std::function<Return()>;

      private:
        callback_t m_func;
        std::shared_ptr<std::promise<Return>> m_result;

      public:
        safe_message(callback_t &&func, std::shared_ptr<std::promise<Return>> result) : m_func(func), m_result(result)
        {
        }

      public:
        ~safe_message() override
        {
            if constexpr (std::is_void_v<Return>)
            {
                m_func();
                m_result->set_value();
            }
            else
            {
                m_result->set_value(m_func());
            }
        }
    };

    template <typename Func>
    auto window::impl::post_safe(Func &&func)
    {
        using return_t = typename decltype(std::function(func))::result_type;

        auto result = std::make_shared<std::promise<return_t>>();
        auto *message = new safe_message<return_t>(std::function{func}, result);

        // ? the WndProc will delete the message after processing.
        PostMessage(hwnd, WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));

        return result->get_future().get();
    }
} // namespace saucer