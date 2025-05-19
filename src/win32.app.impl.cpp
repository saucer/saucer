#include "win32.app.impl.hpp"

namespace saucer
{
    void application::impl::run_once()
    {
        MSG msg{};

        if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            return;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    screen application::impl::convert(MONITORINFOEXW info)
    {
        const auto width  = info.rcMonitor.right - info.rcMonitor.left;
        const auto height = info.rcMonitor.bottom - info.rcMonitor.top;

        return {
            .name     = utils::narrow(info.szDevice),
            .size     = {width, height},
            .position = {info.rcMonitor.top, info.rcMonitor.left},
        };
    }

    LRESULT CALLBACK application::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        if (msg != WM_SAFE_CALL)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        delete reinterpret_cast<safe_message *>(l_param);
        return 0;
    }

    BOOL CALLBACK application::impl::enum_monitor(HMONITOR monitor, HDC, LPRECT, LPARAM user_data)
    {
        auto &rtn = *reinterpret_cast<std::vector<screen> *>(user_data);

        MONITORINFOEXW info{};
        info.cbSize = sizeof(MONITORINFOEXW);

        if (!GetMonitorInfoW(monitor, &info))
        {
            return TRUE;
        }

        if (info.dwFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
        {
            return TRUE;
        }

        rtn.emplace_back(convert(info));

        return TRUE;
    }

    safe_message::safe_message(callback_t callback) : m_callback(std::move(callback)) {}

    safe_message::~safe_message()
    {
        std::invoke(m_callback);
    }
} // namespace saucer
