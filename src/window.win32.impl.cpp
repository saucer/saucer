#include "window.win32.impl.hpp"

namespace saucer
{
    const UINT window::impl::WM_SAFE_CALL = RegisterWindowMessageW(L"safe_call");
    std::atomic<std::size_t> window::impl::open_windows = 0;

    void window::impl::set_background_color(const color_t &color)
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

    std::string window::impl::last_error()
    {
        auto error = GetLastError();

        if (!error)
        {
            return "<No Error>";
        }

        constexpr DWORD dw_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | //
                                   FORMAT_MESSAGE_FROM_SYSTEM |     //
                                   FORMAT_MESSAGE_IGNORE_INSERTS;

        LPWSTR buffer{};
        auto lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        auto size = FormatMessageW(dw_flags, nullptr, error, lang_id, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

        auto message = narrow(std::wstring{buffer, size});
        LocalFree(buffer);

        return message;
    }

    inline std::wstring window::impl::widen(const std::string &str)
    {
        auto size = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);

        if (!size)
        {
            return {};
        }

        std::wstring out(size, 0);
        MultiByteToWideChar(65001, 0, str.c_str(), -1, out.data(), size);

        out.resize(size - 1);
        return out;
    }

    inline std::string window::impl::narrow(const std::wstring &w_str)
    {
        auto size = WideCharToMultiByte(65001, 0, w_str.c_str(), static_cast<int>(w_str.length()), nullptr, 0, nullptr,
                                        nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, out.data(), size, nullptr, nullptr);

        return out;
    }

    inline LRESULT CALLBACK window::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
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