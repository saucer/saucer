#include "window.win32.impl.hpp"

namespace saucer
{
    const UINT window::impl::WM_SAFE_CALL = RegisterWindowMessageW(L"safe_call"); // NOLINT(cert-err58-cpp)
    std::atomic<std::size_t> window::impl::open_windows = 0;

    void window::impl::set_background_color(const color &color)
    {
        background_color = color;

        if (!change_background)
        {
            return;
        }

        change_background();
    }

    bool window::impl::is_thread_safe() const
    {
        return creation_thread == std::this_thread::get_id();
    }

    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        auto *window = reinterpret_cast<saucer::window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!window)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        if (msg == WM_GETMINMAXINFO)
        {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param); // NOLINT(performance-no-int-to-ptr)

            auto [min_x, min_y] = window->min_size();
            auto [max_x, max_y] = window->max_size();

            info->ptMaxTrackSize.x = max_x;
            info->ptMaxTrackSize.y = max_y;

            info->ptMinTrackSize.x = min_x;
            info->ptMinTrackSize.y = min_y;
        }

        if (msg == WM_SIZE)
        {
            window->m_events.at<window_event::resize>().fire(LOWORD(l_param), HIWORD(l_param));
        }

        if (msg == WM_DESTROY || msg == WM_CLOSE)
        {
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
        }

        if (msg == window->window::m_impl->WM_SAFE_CALL)
        {
            delete reinterpret_cast<message *>(l_param); // NOLINT(performance-no-int-to-ptr)
        }

        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
} // namespace saucer
