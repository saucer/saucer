#pragma once
#include "webview.hpp"

#include <wrl.h>
#include <optional>
#include <wil/com.h>
#include <wil/stl.h>
#include <WebView2.h>
#include <wil/win32_helpers.h>

namespace saucer
{
    struct webview::impl
    {
        void init_webview(HWND);

        wil::com_ptr<ICoreWebView2> webview;
        wil::com_ptr<ICoreWebView2Controller> webview_controller;

        bool is_ready{false};
        std::vector<LPCWSTR> injected_scripts;
        std::vector<std::string> scripts_once;
        std::vector<std::string> scripts_on_done;
        std::optional<EventRegistrationToken> resource_requested_token;

        static inline WNDPROC original_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };

    inline void webview::impl::init_webview(HWND hwnd)
    {
        using Microsoft::WRL::Callback;
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        std::wstring temp;
        wil::GetEnvironmentVariableW(L"TEMP", temp);

        CreateCoreWebView2EnvironmentWithOptions(nullptr, temp.c_str(), nullptr,
                                                 Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([=](auto, auto *env) {
                                                     return env->CreateCoreWebView2Controller(
                                                         hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([this](auto, auto *controller) {
                                                                   webview_controller = controller;
                                                                   webview_controller->get_CoreWebView2(&this->webview);
                                                                   return S_OK;
                                                               }).Get());
                                                 }).Get());

        //? Ensure the WebView is created synchronously
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (webview_controller)
            {
                break;
            }
        }

        CoUninitialize();
    }

    inline LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        const auto *webview = reinterpret_cast<class webview *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (webview)
        {
            switch (msg)
            {
            case WM_SHOWWINDOW:
                webview->m_impl->webview_controller->put_IsVisible(static_cast<BOOL>(w_param));
                break;
            case WM_SIZE:
                if (webview->m_impl->webview_controller)
                {
                    webview->m_impl->webview_controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
                }
                break;
            }
        }
        return CallWindowProcW(original_wnd_proc, hwnd, msg, w_param, l_param);
    }
} // namespace saucer