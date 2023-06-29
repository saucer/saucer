#include "utils.win32.hpp"
#include "webview.webview2.impl.hpp"

#include <wrl.h>
#include <regex>
#include <Shlwapi.h>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <wil/win32_helpers.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    using Microsoft::WRL::Callback;

    // NOLINTNEXTLINE(cert-err58-cpp)
    const std::string webview::impl::inject_script = R"js(
        window.saucer = {
            async on_message(message)
            {
                window.chrome.webview.postMessage(message);
            }
        };
        
        window.saucer.on_message("js_ready");
    )js";

    void webview::impl::create_webview(HWND hwnd, const std::wstring &user_folder, bool hardware_accel)
    {
        // ! Whoever chose to use struct names this long: I will find you and I will slap you.

        using controller_completed = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
        using env_completed = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

        auto controller_created = [&](auto, ICoreWebView2Controller *webview_controller) {
            controller = webview_controller;
            controller->get_CoreWebView2(&this->web_view);

            return S_OK;
        };

        auto environment_created = [&](auto, ICoreWebView2Environment *environment) {
            auto handler = Callback<controller_completed>(controller_created);
            return environment->CreateCoreWebView2Controller(hwnd, handler.Get());
        };
        auto created_handler = Callback<env_completed>(environment_created);

        auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();

        if (!hardware_accel)
        {
            options->put_AdditionalBrowserArguments(L"--disable-gpu");
        }

        CreateCoreWebView2EnvironmentWithOptions(nullptr, user_folder.c_str(), options.Get(), created_handler.Get());
    }

    void webview::impl::overwrite_wnd_proc(HWND hwnd)
    {
        auto wnd_proc_ptr = reinterpret_cast<LONG_PTR>(wnd_proc);

        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        original_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, wnd_proc_ptr));
    }

    LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        const auto *web_view = reinterpret_cast<webview *>(userdata); // NOLINT(performance-no-int-to-ptr)

        if (!web_view)
        {
            return 0;
        }

        const auto &impl = web_view->m_impl;

        auto original = [&]() {
            return CallWindowProcW(impl->original_wnd_proc, hwnd, msg, w_param, l_param);
        };

        if (!impl->controller)
        {
            return original();
        }

        if (msg == WM_SHOWWINDOW)
        {
            impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
        }

        if (msg == WM_SIZE)
        {
            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
        }

        return original();
    }

    EventRegistrationToken webview::impl::install_scheme_handler(class webview &parent) const
    {
        auto uri = fmt::format(L"{}*", scheme_prefix_w);
        web_view->AddWebResourceRequestedFilter(uri.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

        auto requested_callback = [&](auto, ICoreWebView2WebResourceRequestedEventArgs *args) {
            wil::com_ptr<ICoreWebView2WebResourceRequest> request;
            args->get_Request(&request);

            auto webview2 = this->web_view.try_query<ICoreWebView2_2>();

            if (!webview2)
            {
                return S_OK;
            }

            wil::com_ptr<ICoreWebView2Environment> env;
            webview2->get_Environment(&env);

            LPWSTR raw_url{};
            request->get_Uri(&raw_url);
            auto url = narrow(raw_url);

            if (url.size() < impl::scheme_prefix.size())
            {
                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                env->CreateWebResourceResponse(nullptr, 500, L"Bad request", L"", &response);

                args->put_Response(response.get());
                return S_OK;
            }

            std::smatch match;
            std::regex_search(url, match, std::regex{fmt::format("{}([^?]+)", scheme_prefix)});

            url = match[1];

            if (!parent.m_embedded_files.contains(url))
            {
                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                env->CreateWebResourceResponse(nullptr, 404, L"Not found", L"", &response);

                args->put_Response(response.get());
                return S_OK;
            }

            const auto &file = parent.m_embedded_files.at(url);

            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
            wil::com_ptr<IStream> data = SHCreateMemStream(file.data, static_cast<UINT>(file.size));

            env->CreateWebResourceResponse(data.get(), 200, L"OK",
                                           fmt::format(L"Content-Type: {}", widen(file.mime)).c_str(), &response);

            args->put_Response(response.get());
            return S_OK;
        };

        using resource_requested = ICoreWebView2WebResourceRequestedEventHandler;
        auto callback = Callback<resource_requested>(requested_callback);

        EventRegistrationToken rtn;
        web_view->add_WebResourceRequested(callback.Get(), &rtn);

        return rtn;
    }
} // namespace saucer