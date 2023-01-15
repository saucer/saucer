#include "webview.webview2.impl.hpp"

#include <wrl.h>
#include <shlwapi.h>
#include <wil/win32_helpers.h>

namespace saucer
{
    using Microsoft::WRL::Callback;

    const std::string webview::impl::inject_script = R"js(
        window.saucer = {
            async on_message(message)
            {
                window.chrome.webview.postMessage(message);
            }
        };
        
        window.saucer.on_message("js_ready");
    )js";

    void webview::impl::overwrite_wnd_proc(HWND hwnd)
    {
        auto wnd_proc_ptr = reinterpret_cast<LONG_PTR>(wnd_proc);
        o_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, wnd_proc_ptr));
    }

    void webview::impl::create_webview(HWND hwnd, const std::wstring &user_folder)
    {
        // ! Whoever chose to use struct names this long: I will find you and I will slap you.

        using controller_completed = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
        using env_completed = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

        auto controller_created = [&](auto, ICoreWebView2Controller *webview_controller) {
            controller = webview_controller;
            controller->get_CoreWebView2(&webview);
        };

        auto environment_created = [&](auto, ICoreWebView2Environment *environment) {
            auto callback = Callback<controller_completed>(controller_created);
            return environment->CreateCoreWebView2Controller(hwnd, callback.Get());
        };

        CreateCoreWebView2EnvironmentWithOptions(nullptr,             //
                                                 user_folder.c_str(), //
                                                 nullptr,             //
                                                 Callback<env_completed>(environment_created).Get());
    }

    LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        const auto *webview = reinterpret_cast<class webview *>(userdata);

        auto original = [&]() { //
            return CallWindowProcW(o_wnd_proc, hwnd, msg, w_param, l_param);
        };

        if (!webview)
        {
            return original();
        }

        const auto &impl = webview->m_impl;

        if (!impl->controller)
        {
            return original();
        }

        if (msg == WM_SHOWWINDOW)
        {
            webview->m_impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
        }

        if (msg == WM_SIZE)
        {
            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
        }

        return original();
    }
} // namespace saucer