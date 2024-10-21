#include "win32.window.impl.hpp"

#include "win32.app.impl.hpp"

namespace saucer
{
    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        auto *window  = reinterpret_cast<saucer::window *>(userdata);

        if (!window)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        const auto &impl = window->m_impl;

        auto original = [&]
        {
            return CallWindowProcW(impl->o_wnd_proc, hwnd, msg, w_param, l_param);
        };

        switch (msg)
        {
        case WM_GETMINMAXINFO: {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param);

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
        case WM_STYLECHANGED: {
            if (w_param != static_cast<WPARAM>(GWL_STYLE))
            {
                break;
            }

            static constexpr auto flag = WS_CAPTION;
            auto *const changes        = reinterpret_cast<STYLESTRUCT *>(l_param);

            if ((changes->styleOld & flag) != (changes->styleNew & flag))
            {
                window->m_events.at<window_event::decorated>().fire(changes->styleNew & flag);
            }

            break;
        }
        case WM_CLOSE: {
            if (window->m_events.at<window_event::close>().until(policy::block))
            {
                return 0;
            }

            auto parent = window->m_parent;

            window->hide();
            window->m_events.at<window_event::closed>().fire();

            auto &instances = parent->native()->instances;
            instances.erase(hwnd);

            if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
            {
                parent->quit();
            }

            return 0;
        }
        }

        return original();
    }
} // namespace saucer
