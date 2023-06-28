#include "window.win32.impl.hpp"

namespace saucer
{
    const UINT window::impl::WM_SAFE_CALL = RegisterWindowMessageW(L"safe_call");
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

    void window::impl::set_dpi_awareness()
    {

        auto *shcore = LoadLibraryW(L"Shcore.dll");
        auto set_process_dpi_awareness = GetProcAddress(shcore, "SetProcessDpiAwareness");

        if (set_process_dpi_awareness)
        {
            reinterpret_cast<HRESULT(CALLBACK *)(DWORD)>(set_process_dpi_awareness)(2);
            return;
        }

        auto *user32 = LoadLibraryW(L"user32.dll");
        auto set_process_dpi_aware = GetProcAddress(user32, "SetProcessDPIAware");

        if (set_process_dpi_aware)
        {
            reinterpret_cast<bool(CALLBACK *)()>(set_process_dpi_aware)();
        }
    }

    LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto *window = reinterpret_cast<class window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!window)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        if (msg == WM_GETMINMAXINFO)
        {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param);

            auto [max_x, max_y] = window->m_impl->max_size;
            auto [min_x, min_y] = window->m_impl->min_size;

            info->ptMaxTrackSize.x = max_x ? max_x : GetSystemMetrics(SM_CXMAXTRACK);
            info->ptMaxTrackSize.y = max_y ? max_y : GetSystemMetrics(SM_CXMAXTRACK);

            info->ptMinTrackSize.x = min_x ? min_x : GetSystemMetrics(SM_CXMINTRACK);
            info->ptMinTrackSize.y = min_y ? min_y : GetSystemMetrics(SM_CXMINTRACK);
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

            if (!(--open_windows))
            {
                PostQuitMessage(0);
            }
        }

        if (msg == window->window::m_impl->WM_SAFE_CALL)
        {
            delete reinterpret_cast<message *>(l_param);
        }

        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
} // namespace saucer