#include "webview.webview2.impl.hpp"

#include "scripts.hpp"
#include "utils.win32.hpp"

#include "window.win32.impl.hpp"
#include "scheme.webview2.impl.hpp"

#include <fmt/core.h>
#include <fmt/xchar.h>

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

    void webview::impl::create_webview(webview *parent, HWND hwnd, saucer::options options)
    {
        auto flags = options.chrome_flags;

        if (!options.hardware_acceleration)
        {
            flags.emplace("--disable-gpu");
        }

        const auto args = fmt::format("{}", fmt::join(flags, " "));
        env_options->put_AdditionalBrowserArguments(utils::widen(args).c_str());

        if (options.storage_path.empty())
        {
            options.storage_path = std::filesystem::temp_directory_path() / "saucer";
        }

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        auto created = [this](auto, auto *result)
        {
            controller = result;

            if (!result || !SUCCEEDED(result->get_CoreWebView2(&this->web_view)))
            {
                utils::throw_error("Failed to create webview2");
            }

            return S_OK;
        };

        auto completed = [this, hwnd, created](auto, auto *env)
        {
            if (!SUCCEEDED(env->CreateCoreWebView2Controller(hwnd, Callback<ControllerCompleted>(created).Get())))
            {
                utils::throw_error("Failed to create webview2 controller");
            }

            return S_OK;
        };

        auto status =
            CreateCoreWebView2EnvironmentWithOptions(nullptr, options.storage_path.wstring().c_str(), env_options.Get(),
                                                     Callback<EnvironmentCompleted>(completed).Get());

        if (!SUCCEEDED(status))
        {
            utils::throw_error("Failed to create webview2");
        }

        while (!controller)
        {
            parent->run<false>();
        }

        auto resource_handler = [this](auto, auto *arg)
        {
            return scheme_handler(arg);
        };

        scheme_token.emplace();
        web_view->add_WebResourceRequested(Callback<ResourceRequested>(resource_handler).Get(), &scheme_token.value());

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
            switch (result.error())
            {
            case request_error::aborted:
                environment->CreateWebResourceResponse(nullptr, 500, L"Aborted", L"", &response);
                break;
            case request_error::bad_url:
                environment->CreateWebResourceResponse(nullptr, 500, L"Invalid Url", L"", &response);
                break;
            case request_error::denied:
                environment->CreateWebResourceResponse(nullptr, 401, L"Unauthorized", L"", &response);
                break;
            case request_error::not_found:
                environment->CreateWebResourceResponse(nullptr, 404, L"Not Found", L"", &response);
                break;
            default:
            case request_error::failed:
                environment->CreateWebResourceResponse(nullptr, 500, L"Failed", L"", &response);
                break;
            }

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

        environment->CreateWebResourceResponse(buffer.Get(), 200, L"OK", combined.c_str(), &response);
        args->put_Response(response.Get());

        return S_OK;
    }

    void webview::impl::overwrite_wnd_proc(HWND hwnd)
    {
        auto ptr          = reinterpret_cast<LONG_PTR>(wnd_proc);
        original_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, ptr));
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
            return CallWindowProcW(impl->original_wnd_proc, hwnd, msg, w_param, l_param);
        };

        if (!impl->controller)
        {
            return original();
        }

        if (msg == web_view->window::m_impl->WM_GET_BACKGROUND)
        {
            auto *message = reinterpret_cast<get_background_message *>(l_param);

            if (ComPtr<ICoreWebView2Controller2> controller; SUCCEEDED(impl->controller.As(&controller)))
            {
                COREWEBVIEW2_COLOR color;
                controller->get_DefaultBackgroundColor(&color);
                message->result->set_value(saucer::color{color.R, color.G, color.B, color.A});
            }

            delete message;

            return original();
        }

        if (msg == web_view->window::m_impl->WM_SET_BACKGROUND)
        {
            auto *message = reinterpret_cast<set_background_message *>(l_param);

            if (ComPtr<ICoreWebView2Controller2> controller; SUCCEEDED(impl->controller.As(&controller)))
            {
                auto [r, g, b, a] = message->data;
                controller->put_DefaultBackgroundColor({.A = a, .R = r, .G = g, .B = b});
            }

            message->result->set_value();
            delete message;

            return original();
        }

        switch (msg)
        {
        case WM_SHOWWINDOW:
            impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
            break;
        case WM_SIZE:
            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
            impl->controller->put_IsVisible(w_param == SIZE_MAXIMIZED || w_param == SIZE_RESTORED);
            break;
        case WM_DESTROY:
            impl->controller->Close();
            break;
        }

        return original();
    }

    template <>
    void webview::impl::setup<web_event::load_finished>(webview *self)
    {
        if (load_token)
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::load_finished>().fire();
            return S_OK;
        };

        load_token.emplace();
        web_view->add_NavigationCompleted(Callback<NavigationComplete>(handler).Get(), &load_token.value());
    }

    template <>
    void webview::impl::setup<web_event::load_started>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::url_changed>(webview *self)
    {
        if (navigation_token)
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::url_changed>().fire(self->url());
            return S_OK;
        };

        navigation_token.emplace();
        web_view->add_SourceChanged(Callback<SourceChanged>(handler).Get(), &navigation_token.value());
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }
} // namespace saucer
