#pragma once

#include "app.impl.hpp"

#include "win32.utils.hpp"

#include <unordered_map>

#include <windows.h>
#include <gdiplus.h>

namespace saucer
{
    struct application::impl::native
    {
        WNDCLASSW wnd_class;
        std::wstring id;

      public:
        HMODULE handle;
        utils::window_handle msg_window;

      public:
        utils::dispatch_controller dispatch_controller{nullptr};
        utils::handle<ULONG_PTR, Gdiplus::GdiplusShutdown> gdi_token;

      public:
        bool quit_on_last_window_closed;
        std::unordered_map<HWND, bool> instances;

      public:
        static void iteration();
        static screen convert(MONITORINFOEXW);

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
        static BOOL CALLBACK enum_monitor(HMONITOR, HDC, LPRECT, LPARAM);

      public:
        static constexpr auto WM_SAFE_CALL = WM_USER + 1;
    };

    class safe_message
    {
        using callback_t = std::move_only_function<void()>;

      private:
        callback_t m_callback;

      public:
        safe_message(callback_t callback);

      public:
        ~safe_message();
    };
} // namespace saucer
