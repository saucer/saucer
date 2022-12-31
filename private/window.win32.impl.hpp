#pragma once
#include "window.hpp"

#include <future>
#include <optional>
#include <Windows.h>

namespace saucer
{
    struct window::impl
    {
        HWND hwnd;
        WNDCLASSW wnd_class;
        std::pair<std::size_t, std::size_t> min_size, max_size;

        bool is_thread_safe() const;
        template <typename Func> auto post_safe(Func &&);
        std::thread::id m_creation_thread = std::this_thread::get_id();

        static inline HMODULE instance;
        static inline std::atomic<std::size_t> open_windows{0};
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
        static const inline UINT WM_SAFE_CALL = RegisterWindowMessageW(L"safe_call");

        static std::wstring widen(const std::string &);
        static std::string narrow(const std::wstring &);
        void set_background_color(const std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> &);

        std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> background_color;
    };

    inline bool window::impl::is_thread_safe() const
    {
        return std::this_thread::get_id() == m_creation_thread;
    }

    class win_call
    {
      public:
        virtual ~win_call() = default;
    };

    template <typename Return> class win_safe_call : public win_call
    {
        using callback_t = std::function<Return()>;

      private:
        callback_t m_func;
        std::promise<Return> &m_result;

      public:
        win_safe_call(callback_t &&func, std::promise<Return> &result) : m_func(func), m_result(result) {}
        ~win_safe_call() override
        {
            m_result.set_value(m_func());
        }
    };

    template <> class win_safe_call<void> : public win_call
    {
        using callback_t = std::function<void()>;

      private:
        callback_t m_func;

      public:
        win_safe_call(callback_t &&func) : m_func(std::move(func)) {}
        ~win_safe_call() override
        {
            m_func();
        }
    };

    template <typename Func> auto window::impl::post_safe(Func &&func)
    {
        using return_t = typename decltype(std::function(func))::result_type;

        if constexpr (std::is_same_v<return_t, void>)
        {
            PostMessage(hwnd, WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new win_safe_call<return_t>(std::function(func))));
        }
        else
        {
            std::promise<return_t> result;
            PostMessage(hwnd, WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(new win_safe_call<return_t>(std::function(func), result)));

            return result.get_future().get();
        }
    }

    inline LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto *window = reinterpret_cast<class window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (window)
        {
            switch (msg)
            {
            case WM_GETMINMAXINFO: {
                auto *info = reinterpret_cast<MINMAXINFO *>(l_param);
                if (window->m_impl->max_size.first)
                {
                    info->ptMaxTrackSize.x = static_cast<int>(window->m_impl->max_size.first);
                }
                if (window->m_impl->max_size.second)
                {
                    info->ptMaxTrackSize.y = static_cast<int>(window->m_impl->max_size.second);
                }
                if (window->m_impl->min_size.first)
                {
                    info->ptMinTrackSize.x = static_cast<int>(window->m_impl->min_size.first);
                }
                if (window->m_impl->min_size.second)
                {
                    info->ptMinTrackSize.y = static_cast<int>(window->m_impl->min_size.second);
                }
            }
            break;
            case WM_SIZE:
                window->m_events.at<window_event::resize>().fire(LOWORD(l_param), HIWORD(l_param));
                break;
            case WM_DESTROY:
            case WM_CLOSE:
                for (const auto &result : window->m_events.at<window_event::close>().fire())
                {
                    if (result)
                    {
                        return 0;
                    }
                }
                open_windows--;
                if (!open_windows)
                {
                    PostQuitMessage(0);
                }
                break;
            }

            if (msg == window->window::m_impl->WM_SAFE_CALL)
            {
                delete reinterpret_cast<win_call *>(l_param);
            }
        }

        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    inline std::string window::impl::narrow(const std::wstring &w_str)
    {
        auto sz = WideCharToMultiByte(65001, 0, w_str.c_str(), static_cast<int>(w_str.length()), nullptr, 0, nullptr, nullptr);
        if (!sz)
        {
            return {};
        }

        std::string out(sz, 0);
        WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, out.data(), sz, nullptr, nullptr);
        return out;
    }

    inline std::wstring window::impl::widen(const std::string &str)
    {
        auto wsz = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);
        if (!wsz)
        {
            return {};
        }

        std::wstring out(wsz, 0);
        MultiByteToWideChar(65001, 0, str.c_str(), -1, out.data(), wsz);
        out.resize(wsz - 1);
        return out;
    }

    inline void window::impl::enable_transparency(bool enabled, bool blur, bool normal_blur)
    {
        const auto &[r, g, b, a] = background_color;
        transparency_enabled = enabled;
        blur_enabled = blur;

        accent_policy policy;
        policy.flags = 2;
        policy.animation_id = 0;
        policy.gradient_color = RGB(r, g, b) | static_cast<BYTE>(a) << 24;

        if (enabled)
        {
            policy.state = accent_state::transparent_gradient;
        }
        if (blur)
        {
            policy.state = normal_blur ? accent_state::blur_behind : accent_state::acrylic_blur_behind;
        }
        if (!enabled)
        {
            policy.state = accent_state::disabled;
        }

        composition_data data;
        data.policy = &policy;
        data.data_size = sizeof(policy);
        data.attribute = composition_attribute::accent_policy;

        SetWindowCompositionAttribute(hwnd, &data);
    }

    inline void window::impl::set_background_color(const std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> &color)
    {
        background_color = color;
        enable_transparency(transparency_enabled, blur_enabled);
    }
} // namespace saucer