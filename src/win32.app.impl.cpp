#include "win32.app.impl.hpp"

namespace saucer
{
    using native = application::impl::native;

    void native::iteration()
    {
        MSG msg{};

        if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            return;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    screen native::convert(MONITORINFOEXW info)
    {
        return {
            .name     = utils::narrow(info.szDevice),
            .size     = {.w = info.rcMonitor.right - info.rcMonitor.left, .h = info.rcMonitor.bottom - info.rcMonitor.top},
            .position = {.x = info.rcMonitor.top, .y = info.rcMonitor.left},
        };
    }

    LRESULT CALLBACK native::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        if (msg != WM_SAFE_CALL)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        delete reinterpret_cast<safe_message *>(l_param);

        return 0;
    }

    BOOL CALLBACK native::enum_monitor(HMONITOR monitor, HDC, LPRECT, LPARAM user_data)
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
