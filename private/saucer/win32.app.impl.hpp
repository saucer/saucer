#pragma once

#include "app.hpp"

#include "win32.utils.hpp"

#include <unordered_map>

#include <windows.h>
#include <gdiplus.h>

namespace saucer
{
    struct application::impl
    {
        WNDCLASSW wnd_class;

      public:
        HMODULE handle;
        std::wstring id;

      public:
        DWORD thread;
        utils::win_handle<ULONG_PTR, Gdiplus::GdiplusShutdown> gdi_token;

      public:
        std::unordered_map<HWND, bool> instances;

      public:
        static void process(MSG);
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

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
