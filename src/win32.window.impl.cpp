#include "win32.window.impl.hpp"

namespace saucer
{
    const UINT window::impl::WM_SAFE_CALL            = RegisterWindowMessageW(L"safe_call");
    std::atomic<std::size_t> window::impl::instances = 0;

    bool window::impl::is_thread_safe()
    {
        return instance;
    }

    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        if (msg == impl::WM_SAFE_CALL)
        {
            delete reinterpret_cast<safe_message *>(l_param);
            return 0;
        }

        auto original = [&]()
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        };

        auto *window = reinterpret_cast<saucer::window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!window)
        {
            return original();
        }

        switch (msg)
        {
        case WM_GETMINMAXINFO: {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param);

            if (!window->m_impl->resizable)
            {
                auto [width, height] = window->size();

                info->ptMaxTrackSize = {.x = width, .y = height};
                info->ptMinTrackSize = info->ptMaxTrackSize;

                return original();
            }

            if (auto min_size = window->m_impl->min_size; min_size)
            {
                auto [min_x, min_y]  = min_size.value();
                info->ptMinTrackSize = {.x = min_x, .y = min_y};
            }

            if (auto max_size = window->m_impl->max_size; max_size)
            {
                auto [max_x, max_y]  = max_size.value();
                info->ptMaxTrackSize = {.x = max_x, .y = max_y};
            }

            break;
        }
        case WM_NCACTIVATE:
            window->m_events.at<window_event::focus>().fire(w_param);
            break;
        case WM_SIZE: {
            switch (w_param)
            {
            case SIZE_MAXIMIZED:
                window->m_impl->prev_state = SIZE_MAXIMIZED;
                window->m_events.at<window_event::maximize>().fire(true);
                break;
            case SIZE_MINIMIZED:
                window->m_impl->prev_state = SIZE_MINIMIZED;
                window->m_events.at<window_event::minimize>().fire(true);
                break;
            case SIZE_RESTORED:
                switch (window->m_impl->prev_state)
                {
                case SIZE_MAXIMIZED:
                    window->m_events.at<window_event::maximize>().fire(false);
                    break;
                case SIZE_MINIMIZED:
                    window->m_events.at<window_event::minimize>().fire(false);
                    break;
                }

                window->m_impl->prev_state = SIZE_RESTORED;
                break;
            }

            auto [width, height] = window->size();
            window->m_events.at<window_event::resize>().fire(width, height);

            break;
        }
        case WM_CLOSE: {
            if (window->m_events.at<window_event::close>().until(true))
            {
                return 0;
            }

            window->m_events.at<window_event::closed>().fire();

            if (--instances > 0)
            {
                break;
            }

            PostQuitMessage(0);
            break;
        }
        }

        return original();
    }

    safe_message::safe_message(callback_t callback) : m_callback(std::move(callback)) {}

    safe_message::~safe_message()
    {
        std::invoke(m_callback);
    }
} // namespace saucer
