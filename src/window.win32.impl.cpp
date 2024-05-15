#include "window.win32.impl.hpp"

#include <ranges>

namespace saucer
{
    const UINT window::impl::WM_SAFE_CALL            = RegisterWindowMessageW(L"safe_call");
    std::atomic<std::size_t> window::impl::instances = 0;

    bool window::impl::is_thread_safe() const
    {
        return creation_thread == std::this_thread::get_id();
    }

    std::pair<int, int> window::impl::window_offset() const
    {
        RECT window_rect;
        RECT client_rect;

        GetWindowRect(hwnd, &window_rect);
        GetClientRect(hwnd, &client_rect);

        int width  = window_rect.right - window_rect.left - client_rect.right;
        int height = window_rect.bottom - window_rect.top - client_rect.bottom;

        return {width, height};
    }

    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto *window = reinterpret_cast<saucer::window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        auto original = [&]()
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        };

        if (!window)
        {
            return original();
        }

        if (msg == window->window::m_impl->WM_SAFE_CALL)
        {
            delete reinterpret_cast<message *>(l_param);
            return original();
        }

        switch (msg)
        {
        case WM_GETMINMAXINFO: {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param);

            auto [min_x, min_y] = window->min_size();
            auto [max_x, max_y] = window->max_size();

            info->ptMaxTrackSize.x = max_x;
            info->ptMaxTrackSize.y = max_y;

            info->ptMinTrackSize.x = min_x;
            info->ptMinTrackSize.y = min_y;

            break;
        }
        case WM_NCACTIVATE:
            window->m_events.at<window_event::focus>().fire(w_param);
            break;
        case WM_SIZE: {
            switch (w_param)
            {
            case SIZE_MAXIMIZED:
                window->m_impl->last_state = SIZE_MAXIMIZED;
                window->m_events.at<window_event::maximize>().fire(true);
                break;
            case SIZE_MINIMIZED:
                window->m_impl->last_state = SIZE_MINIMIZED;
                window->m_events.at<window_event::minimize>().fire(true);
                break;
            case SIZE_RESTORED:
                switch (window->m_impl->last_state)
                {
                case SIZE_MAXIMIZED:
                    window->m_events.at<window_event::maximize>().fire(false);
                    break;
                case SIZE_MINIMIZED:
                    window->m_events.at<window_event::minimize>().fire(false);
                    break;
                }

                window->m_impl->last_state = SIZE_RESTORED;
                break;
            }

            auto [width, height] = window->size();
            window->m_events.at<window_event::resize>().fire(width, height);

            break;
        }
        case WM_DESTROY:
        case WM_CLOSE: {
            if (window->m_events.at<window_event::close>().until(true))
            {
                return 0;
            }

            window->m_events.at<window_event::closed>().fire();
            instances--;

            if (instances > 0)
            {
                break;
            }

            PostQuitMessage(0);
            break;
        }
        }

        return original();
    }
} // namespace saucer
