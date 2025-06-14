#include "win32.window.impl.hpp"

#include "win32.app.impl.hpp"

namespace saucer
{
    void window::impl::set_style(HWND hwnd, long style)
    {
        auto current = GetWindowLongPtr(hwnd, GWL_STYLE);

        if (current & WS_VISIBLE)
        {
            style |= WS_VISIBLE;
        }

        SetWindowLongPtr(hwnd, GWL_STYLE, style);
    }

    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        auto *window  = reinterpret_cast<saucer::window *>(userdata);

        if (!window)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        const auto &impl = window->m_impl;

        switch (msg)
        {
        case WM_STYLECHANGED: {
            auto &prev         = window->m_impl->prev_decoration;
            const auto current = window->decoration();

            if (prev.has_value() && prev.value() == current)
            {
                break;
            }

            prev.emplace(current);
            window->m_events.get<window_event::decorated>().fire(current);

            break;
        }
        case WM_NCCALCSIZE:
            if (!window->m_impl->titlebar)
            {
                auto *const rect = w_param ? &reinterpret_cast<NCCALCSIZE_PARAMS *>(l_param)->rgrc[0] //
                                           : reinterpret_cast<RECT *>(l_param);

                const auto maximized = window->maximized();
                const auto keep      = !maximized || rect->top >= 0;

                WINDOWINFO info{};
                GetWindowInfo(hwnd, &info);

                rect->top += keep ? (maximized ? 0 : 1) : static_cast<LONG>(info.cxWindowBorders);
                rect->bottom -= static_cast<LONG>(info.cyWindowBorders);

                rect->left += static_cast<LONG>(info.cxWindowBorders);
                rect->right -= static_cast<LONG>(info.cxWindowBorders);

                return 0;
            }
            break;
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
            window->m_events.get<window_event::focus>().fire(w_param);
            break;
        case WM_SIZE: {
            switch (w_param)
            {
            case SIZE_MAXIMIZED:
                window->m_events.get<window_event::maximize>().fire(true);
                break;
            case SIZE_MINIMIZED:
                window->m_events.get<window_event::minimize>().fire(true);
                break;
            case SIZE_RESTORED:
                switch (window->m_impl->prev_state)
                {
                case SIZE_MAXIMIZED:
                    window->m_events.get<window_event::maximize>().fire(false);
                    break;
                case SIZE_MINIMIZED:
                    window->m_events.get<window_event::minimize>().fire(false);
                    break;
                }
                break;
            }

            window->m_impl->prev_state = w_param;

            auto [width, height] = window->size();
            window->m_events.get<window_event::resize>().fire(width, height);

            break;
        }
        case WM_CLOSE: {
            if (window->m_events.get<window_event::close>().fire().find(policy::block))
            {
                return 0;
            }

            auto *const parent = window->m_parent;
            auto *const native = parent->native<false>();

            auto &instances = native->instances;

            window->hide();
            instances.erase(hwnd);
            window->m_events.get<window_event::closed>().fire();

            if (!native->quit_on_last_window_closed)
            {
                return 0;
            }

            if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
            {
                parent->quit();
            }

            return 0;
        }
        }

        return CallWindowProcW(impl->hook.original(), hwnd, msg, w_param, l_param);
    }
} // namespace saucer
