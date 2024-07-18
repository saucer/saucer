#include "utils.win32.hpp"
#include "webview.webview2.impl.hpp"

#include <fmt/core.h>
#include <fmt/xchar.h>

#include <shlwapi.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    constinit std::string_view webview::impl::inject_script = R"js(
        window.saucer = 
        {
            window_edge:
            {
                top:    1,
                bottom: 2,
                left:   4,
                right:  8,
            },
            on_message: async (message) =>
            {
                window.chrome.webview.postMessage(message);
            },
            start_drag: async () =>
            {
                await window.saucer.on_message(JSON.stringify({
                    ["saucer:drag"]: true
                }));
            },
            start_resize: async (edge) =>
            {
                await window.saucer.on_message(JSON.stringify({
                    ["saucer:resize"]: true,
                    edge,
                }));
            }
        };
    )js";

    void webview::impl::install_scheme_handler(webview *parent)
    {
        static const auto scheme_prefix_w = utils::widen(std::string{scheme_prefix});

        auto uri    = fmt::format(L"{}*", scheme_prefix_w);
        auto status = web_view->AddWebResourceRequestedFilter(uri.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

        if (!SUCCEEDED(status))
        {
            utils::throw_error("Failed to add resource filter");
        }

        auto handler = [&, parent](auto, auto *args)
        {
            ComPtr<ICoreWebView2WebResourceRequest> request;
            args->get_Request(&request);

            ComPtr<ICoreWebView2_2> webview2_2;
            web_view->QueryInterface(IID_PPV_ARGS(&webview2_2));

            if (!webview2_2)
            {
                return S_FALSE;
            }

            ComPtr<ICoreWebView2Environment> environment;
            webview2_2->get_Environment(&environment);

            LPWSTR raw{};
            request->get_Uri(&raw);

            auto url = utils::narrow(raw);
            CoTaskMemFree(raw);

            if (!url.starts_with(scheme_prefix))
            {
                ComPtr<ICoreWebView2WebResourceResponse> response;
                environment->CreateWebResourceResponse(nullptr, 500, L"Bad Request", L"", &response);

                args->put_Response(response.Get());
                return S_OK;
            }

            url = url.substr(scheme_prefix.size());
            url = url.substr(0, url.find_first_of('?'));

            if (!parent->m_embedded_files.contains(url))
            {
                ComPtr<ICoreWebView2WebResourceResponse> response;
                environment->CreateWebResourceResponse(nullptr, 404, L"Not Found", L"", &response);

                args->put_Response(response.Get());
                return S_OK;
            }

            const auto &file = parent->m_embedded_files.at(url);

            ComPtr<ICoreWebView2WebResourceResponse> response;
            ComPtr<IStream> data = SHCreateMemStream(file.content.data(), static_cast<UINT>(file.content.size()));

            environment->CreateWebResourceResponse(
                data.Get(), 200, L"OK", fmt::format(L"Content-Type: {}", utils::widen(file.mime)).c_str(), &response);

            args->put_Response(response.Get());
            return S_OK;
        };

        auto callback = mcb{handler};
        web_view->add_WebResourceRequested(callback, &scheme_handler);
    }

    void webview::impl::create_webview(webview *parent, HWND hwnd, saucer::options options)
    {
        auto env_options = Make<CoreWebView2EnvironmentOptions>();
        ComPtr<ICoreWebView2EnvironmentOptions4> env_options4;

        if (!SUCCEEDED(env_options->QueryInterface(IID_PPV_ARGS(&env_options4))))
        {
            utils::throw_error("Failed to query ICoreWebView2EnvironmentOptions4");
        }

        static const WCHAR *allowed_origins[1] = {L"*"};
        auto scheme                            = Make<CoreWebView2CustomSchemeRegistration>(L"saucer");

        scheme->put_TreatAsSecure(true);
        scheme->put_HasAuthorityComponent(true);
        scheme->SetAllowedOrigins(1, allowed_origins);

        ICoreWebView2CustomSchemeRegistration *registration[1] = {scheme.Get()};

        if (!SUCCEEDED(env_options4->SetCustomSchemeRegistrations(1, registration)))
        {
            utils::throw_error("Failed to register custom scheme");
        }

        auto flags = options.chrome_flags;

        if (!options.hardware_acceleration)
        {
            flags.emplace_back("--disable-gpu");
        }

        const auto args = fmt::format("{}", fmt::join(flags, " "));
        env_options->put_AdditionalBrowserArguments(utils::widen(args).c_str());

        if (options.storage_path.empty())
        {
            options.storage_path = std::filesystem::temp_directory_path();
        }

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        auto controller_created = mcb{[&](auto, auto *webview_controller)
                                      {
                                          controller = webview_controller;
                                          auto rtn   = controller->get_CoreWebView2(&this->web_view);

                                          if (!SUCCEEDED(rtn))
                                          {
                                              utils::throw_error("Failed to create webview2");
                                          }

                                          return rtn;
                                      }};

        auto created = mcb{[&](auto, auto *env)
                           {
                               auto rtn = env->CreateCoreWebView2Controller(hwnd, controller_created);

                               if (!SUCCEEDED(rtn))
                               {
                                   utils::throw_error("Failed to create webview2 controller");
                               }

                               return rtn;
                           }};

        auto status = CreateCoreWebView2EnvironmentWithOptions(nullptr, options.storage_path.wstring().c_str(),
                                                               env_options.Get(), created);

        if (!SUCCEEDED(status))
        {
            utils::throw_error("Failed to create webview2");
        }

        while (!controller)
        {
            parent->run<false>();
        }

        CoUninitialize();
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

        switch (msg)
        {
        case WM_SHOWWINDOW:
            impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
            break;
        case WM_SIZE:
            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
            impl->controller->put_IsVisible(w_param == SIZE_MAXIMIZED || w_param == SIZE_RESTORED);
            break;
        }

        return original();
    }

    template <>
    void webview::impl::setup<web_event::load_finished>(webview *self)
    {
        if (load_finished.value > 0)
        {
            return;
        }

        auto handler = mcb{[self](auto...)
                           {
                               self->m_events.at<web_event::load_finished>().fire();
                               return S_OK;
                           }};

        web_view->add_NavigationCompleted(handler, &load_finished);
    }

    template <>
    void webview::impl::setup<web_event::load_started>(webview *self)
    {
    }

    template <>
    void webview::impl::setup<web_event::url_changed>(webview *self)
    {
        if (url_changed.value > 0)
        {
            return;
        }

        auto handler = mcb{[self](auto...)
                           {
                               self->m_events.at<web_event::url_changed>().fire(self->url());
                               return S_OK;
                           }};

        web_view->add_SourceChanged(handler, &url_changed);
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *self)
    {
    }
} // namespace saucer
