#pragma once

#include "window.hpp"

#include <optional>

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
        std::optional<std::pair<int, int>> max_size, min_size;

      public:
        [[nodiscard]] bool is_thread_safe() const;
        [[nodiscard]] std::pair<int, int> window_offset() const;

      public:
        static const UINT WM_SAFE_CALL;
        static const UINT WM_GET_BACKGROUND;
        static const UINT WM_SET_BACKGROUND;

      public:
        static std::atomic<std::size_t> instances;

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

      public:
        template <typename T>
        auto post(T *, UINT);

      public:
        template <typename Func, typename Result = std::invoke_result_t<Func>>
        Result post_safe(Func &&);
    };

    struct set_background_message
    {
        color data;
        std::promise<void> *result;
    };

    struct get_background_message
    {
        std::promise<color> *result;
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
        safe_message(callback_t func, std::promise<T> *result) : m_func(std::move(func)), m_result(result) {}

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

    template <typename T>
    auto window::impl::post(T *data, UINT msg)
    {
        PostMessage(hwnd, msg, 0, reinterpret_cast<LPARAM>(data));
    }

    template <typename Func, typename Result>
    Result window::impl::post_safe(Func &&func)
    {
        if (!hwnd)
        {
            return Result{};
        }

        std::promise<Result> result;

        // ? The WndProc will delete the message.
        post(new safe_message<Result>{std::forward<Func>(func), &result}, WM_SAFE_CALL);

        return result.get_future().get();
    }
} // namespace saucer
