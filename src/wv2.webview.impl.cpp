#include "wv2.webview.impl.hpp"

#include "scripts.hpp"
#include "win32.utils.hpp"

#include "win32.window.impl.hpp"
#include "wv2.scheme.impl.hpp"

#include <cassert>

#include <fmt/core.h>
#include <fmt/xchar.h>
#include <rebind/enum.hpp>

#include <windows.h>
#include <gdiplus.h>

#include <shlwapi.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    const std::string &webview::impl::inject_script()
    {
        static std::optional<std::string> instance;

        if (instance)
        {
            return instance.value();
        }

        instance.emplace(fmt::format(scripts::webview_script, fmt::arg("internal", R"js(
        send_message: async (message) =>
        {
            window.chrome.webview.postMessage(message);
        }
        )js")));

        return instance.value();
    }

    void webview::impl::set_wnd_proc(HWND hwnd)
    {
        auto ptr   = reinterpret_cast<LONG_PTR>(wnd_proc);
        o_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, ptr));
    }

    void webview::impl::create_webview(webview *parent, HWND hwnd, saucer::preferences preferences)
    {
        if (!preferences.hardware_acceleration)
        {
            preferences.browser_flags.emplace("--disable-gpu");
        }

        const auto args = fmt::format("{}", fmt::join(preferences.browser_flags, " "));
        env_options->put_AdditionalBrowserArguments(utils::widen(args).c_str());

        if (preferences.storage_path.empty())
        {
            preferences.storage_path = std::filesystem::temp_directory_path() / "saucer";
        }

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        auto created = [this](auto, auto *result)
        {
            controller = result;

            if (!result || !SUCCEEDED(result->get_CoreWebView2(&this->web_view)))
            {
                assert(false && "Failed to get CoreWebView2");
            }

            return S_OK;
        };

        auto completed = [this, hwnd, created](auto, auto *env)
        {
            if (!SUCCEEDED(env->CreateCoreWebView2Controller(hwnd, Callback<ControllerCompleted>(created).Get())))
            {
                assert(false && "Failed to create WebView2 controller");
            }

            return S_OK;
        };

        auto status = CreateCoreWebView2EnvironmentWithOptions(nullptr, preferences.storage_path.wstring().c_str(),
                                                               env_options.Get(),
                                                               Callback<EnvironmentCompleted>(completed).Get());

        if (!SUCCEEDED(status))
        {
            assert(false && "Failed to create WebView2");
        }

        while (!controller)
        {
            parent->run<false>();
        }

        auto resource_handler = [this](auto, auto *arg)
        {
            return scheme_handler(arg);
        };

        CoUninitialize();
    }

    HRESULT webview::impl::scheme_handler(ICoreWebView2WebResourceRequestedEventArgs *args)
    {
        ComPtr<ICoreWebView2WebResourceRequest> request;

        if (!SUCCEEDED(args->get_Request(&request)))
        {
            return S_OK;
        }

        ComPtr<IStream> body;

        if (!SUCCEEDED(request->get_Content(&body)))
        {
            return S_OK;
        }

        ComPtr<ICoreWebView2Environment> environment;

        if (ComPtr<ICoreWebView2_2> web_view2;
            !SUCCEEDED(web_view.As(&web_view2)) || !SUCCEEDED(web_view2->get_Environment(&environment)))
        {
            return S_OK;
        }

        auto req = saucer::request{{args, request, body}};

        auto url = req.url();
        auto end = url.find(':');

        if (end == std::string::npos)
        {
            return S_OK;
        }

        auto scheme = schemes.find(url.substr(0, end));

        if (scheme == schemes.end())
        {
            return S_OK;
        }

        auto result = std::invoke(scheme->second, req);
        ComPtr<ICoreWebView2WebResourceResponse> response;

        if (!result.has_value())
        {
            auto error = result.error();
            auto meta  = rebind::enum_value(error);
            auto name  = meta ? meta->name : "unknown";

            environment->CreateWebResourceResponse(nullptr, std::to_underlying(error),
                                                   utils::widen(std::string{name}).c_str(), L"", &response);

            return args->put_Response(response.Get());
        }

        auto data = result->data;
        ComPtr<IStream> buffer =
            SHCreateMemStream(reinterpret_cast<const BYTE *>(data.data()), static_cast<UINT>(data.size()));

        std::vector<std::string> headers{fmt::format("Content-Type: {}", result->mime)};

        for (const auto &[name, value] : result->headers)
        {
            headers.emplace_back(fmt::format("{}: {}", name, value));
        }

        auto combined = utils::widen(fmt::format("{}", fmt::join(headers, "\n")));

        environment->CreateWebResourceResponse(buffer.Get(), result->status, L"OK", combined.c_str(), &response);
        args->put_Response(response.Get());

        return S_OK;
    }

    LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata        = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        const auto *web_view = reinterpret_cast<webview *>(userdata);

        if (!web_view)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        const auto &impl = web_view->m_impl;

        auto original = [&]()
        {
            return CallWindowProcW(impl->o_wnd_proc, hwnd, msg, w_param, l_param);
        };

        if (!impl->controller)
        {
            return original();
        }

        switch (msg)
        {
        case WM_SHOWWINDOW:
            impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
            break;
        case WM_SIZE: {
            if (w_param == SIZE_MAXIMIZED || w_param == SIZE_RESTORED)
            {
                impl->controller->put_IsVisible(true);
            }

            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
            break;
        }
        case WM_DESTROY:
            impl->controller->Close();
            break;
        case WM_QUIT:
            if (gdi_token)
            {
                Gdiplus::GdiplusShutdown(gdi_token);
            }
            break;
        }

        return original();
    }

    template <>
    void webview::impl::setup<web_event::title_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::title_changed>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::title_changed>().fire(self->page_title());
            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_DocumentTitleChanged(Callback<TitleChanged>(handler).Get(), &token);
        event.on_clear([self, token]() { self->m_impl->web_view->remove_DocumentTitleChanged(token); });
    }

    template <>
    void webview::impl::setup<web_event::load_finished>(webview *self)
    {
        auto &event = self->m_events.at<web_event::load_finished>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::load_finished>().fire();
            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_NavigationCompleted(Callback<NavigationComplete>(handler).Get(), &token);
        event.on_clear([self, token]() { self->m_impl->web_view->remove_NavigationCompleted(token); });
    }

    template <>
    void webview::impl::setup<web_event::icon_changed>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::load_started>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::url_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::url_changed>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::url_changed>().fire(self->url());
            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_SourceChanged(Callback<SourceChanged>(handler).Get(), &token);
        event.on_clear([self, token]() { self->m_impl->web_view->remove_SourceChanged(token); });
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }
} // namespace saucer
