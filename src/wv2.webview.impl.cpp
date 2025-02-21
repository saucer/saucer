#include "wv2.webview.impl.hpp"

#include "scripts.hpp"
#include "request.hpp"

#include "win32.utils.hpp"
#include "win32.app.impl.hpp"
#include "wv2.scheme.impl.hpp"

#include <cassert>

#include <rebind/utils/enum.hpp>

#include <fmt/core.h>
#include <fmt/xchar.h>

#include <windows.h>
#include <gdiplus.h>

#include <shlwapi.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    const std::string &webview::impl::inject_script()
    {
        static constexpr auto internal = R"js(
            message: async (message) =>
            {
                window.chrome.webview.postMessage(message);
            }
        )js";

        static const auto script = fmt::format(scripts::webview_script,            //
                                               fmt::arg("internal", internal),     //
                                               fmt::arg("stubs", request::stubs()) //
        );

        return script;
    }

    ComPtr<CoreWebView2EnvironmentOptions> webview::impl::env_options()
    {
        static ComPtr<CoreWebView2EnvironmentOptions> instance;

        if (!instance)
        {
            instance = Make<CoreWebView2EnvironmentOptions>();
        }

        return instance;
    }

    void webview::impl::create_webview(const std::shared_ptr<application> &app, HWND hwnd, preferences prefs)
    {
        if (!prefs.hardware_acceleration)
        {
            prefs.browser_flags.emplace("--disable-gpu");
        }

        const auto args        = fmt::format("{}", fmt::join(prefs.browser_flags, " "));
        const auto env_options = impl::env_options();

        env_options->put_AdditionalBrowserArguments(utils::widen(args).c_str());

        if (prefs.persistent_cookies && prefs.storage_path.empty())
        {
            prefs.storage_path = fs::current_path() / ".saucer";

            std::error_code ec{};
            fs::create_directories(prefs.storage_path, ec);

            SetFileAttributesW(prefs.storage_path.wstring().c_str(), FILE_ATTRIBUTE_HIDDEN);
        }
        else if (prefs.storage_path.empty())
        {
            static constexpr auto hash_size = 32;

            auto id   = app->native<false>()->id;
            auto hash = id;

            if (BYTE data[hash_size]{}; HashData(reinterpret_cast<BYTE *>(id.data()), id.size(), data, hash_size) == S_OK)
            {
                hash = data                                                                    //
                       | std::views::transform([](auto x) { return fmt::format(L"{:x}", x); }) //
                       | std::views::join                                                      //
                       | std::ranges::to<std::wstring>();
            }
            else
            {
                assert(false && "Failedd to compute hash of id");
            }

            prefs.storage_path = std::filesystem::temp_directory_path() / fmt::format(L"saucer-{}", hash);
            temp_path          = prefs.storage_path;
        }

        auto created = [this](auto, auto *result)
        {
            controller = result;

            ComPtr<ICoreWebView2> webview;

            if (!result || !SUCCEEDED(result->get_CoreWebView2(&webview)))
            {
                assert(false && "Failed to get CoreWebView2");
            }

            if (!SUCCEEDED(webview.As(&web_view)))
            {
                assert(false && "Failed to get CoreWebView2_2");
            }

            return S_OK;
        };

        auto completed = [hwnd, created](auto, auto *env)
        {
            if (!SUCCEEDED(env->CreateCoreWebView2Controller(hwnd, Callback<ControllerCompleted>(created).Get())))
            {
                assert(false && "Failed to create WebView2 controller");
            }

            return S_OK;
        };

        const auto storage_path = prefs.storage_path.wstring();
        const auto status       = CreateCoreWebView2EnvironmentWithOptions(nullptr, storage_path.c_str(), env_options.Get(),
                                                                           Callback<EnvironmentCompleted>(completed).Get());

        if (!SUCCEEDED(status))
        {
            assert(false && "Failed to create WebView2");
        }

        while (!controller)
        {
            app->run<false>();
        }

        web_view->get_BrowserProcessId(&browser_pid);
    }

    HRESULT webview::impl::scheme_handler(ICoreWebView2WebResourceRequestedEventArgs *args, webview *self)
    {
        ComPtr<ICoreWebView2WebResourceRequest> request;

        if (!SUCCEEDED(args->get_Request(&request)))
        {
            return S_OK;
        }

        utils::string_handle raw;
        request->get_Uri(&raw.reset());

        auto url = utils::narrow(raw.get());
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

        ComPtr<ICoreWebView2Environment> environment;

        if (!SUCCEEDED(web_view->get_Environment(&environment)))
        {
            return S_OK;
        }

        ComPtr<ICoreWebView2Deferral> deferral;

        if (!SUCCEEDED(args->GetDeferral(&deferral)))
        {
            return S_OK;
        }

        ComPtr<IStream> content;

        if (!SUCCEEDED(request->get_Content(&content)))
        {
            return S_OK;
        }

        auto resolve = [environment, args, deferral](const scheme::response &response)
        {
            const auto *raw = reinterpret_cast<const BYTE *>(response.data.data());
            const auto size = static_cast<const UINT>(response.data.size());

            ComPtr<IStream> buffer           = SHCreateMemStream(raw, size);
            std::vector<std::string> headers = {fmt::format("Content-Type: {}", response.mime)};

            for (const auto &[name, value] : response.headers)
            {
                headers.emplace_back(fmt::format("{}: {}", name, value));
            }

            const auto combined = utils::widen(fmt::format("{}", fmt::join(headers, "\n")));

            ComPtr<ICoreWebView2WebResourceResponse> result;
            environment->CreateWebResourceResponse(buffer.Get(), response.status, L"OK", combined.c_str(), &result);

            args->put_Response(result.Get());
            deferral->Complete();
        };

        auto reject = [environment, args, deferral](const scheme::error &error)
        {
            auto name  = rebind::utils::find_enum_name(error).value_or("unknown");
            auto value = std::to_underlying(error);

            ComPtr<ICoreWebView2WebResourceResponse> result;
            environment->CreateWebResourceResponse(nullptr, value, utils::widen(std::string{name}).c_str(), L"", &result);

            args->put_Response(result.Get());
            deferral->Complete();
        };

        auto forward = [self]<typename T>(T &&callback)
        {
            return [self, callback = std::forward<T>(callback)]<typename... Ts>(Ts &&...args) mutable
            {
                self->m_parent->post([callback = std::forward<T>(callback), ... args = std::forward<Ts>(args)]() mutable
                                     { std::invoke(callback, std::forward<Ts>(args)...); });
            };
        };

        auto &[resolver, policy] = scheme->second;

        auto req      = scheme::request{{request, content}};
        auto executor = scheme::executor{forward(std::move(resolve)), forward(std::move(reject))};

        if (policy != launch::async)
        {
            std::invoke(resolver, std::move(req), std::move(executor));
            return S_OK;
        }

        self->m_parent->pool().emplace([resolver, executor = std::move(executor), req = std::move(req)]() mutable
                                       { std::invoke(resolver, std::move(req), std::move(executor)); });

        return S_OK;
    }

    LRESULT CALLBACK webview::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto userdata       = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        const auto *webview = reinterpret_cast<saucer::webview *>(userdata);

        if (!webview || !webview->m_impl->controller)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        const auto &impl = webview->m_impl;

        switch (msg)
        {
        case WM_SHOWWINDOW:
            impl->controller->put_IsVisible(static_cast<BOOL>(w_param));
            break;
        case WM_SIZE:
            impl->controller->put_IsVisible(w_param != SIZE_MINIMIZED);
            impl->controller->put_Bounds(RECT{0, 0, LOWORD(l_param), HIWORD(l_param)});
            break;
        }

        return CallWindowProcW(impl->o_wnd_proc, hwnd, msg, w_param, l_param);
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::navigated>(webview *self)
    {
        auto &event = self->m_events.at<web_event::navigated>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            auto url = self->url();
            self->m_parent->post([self, url] { self->m_events.at<web_event::navigated>().fire(url); });

            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_SourceChanged(Callback<SourceChanged>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_SourceChanged(token); });
    }

    template <>
    void webview::impl::setup<web_event::navigate>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::favicon>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::title>(webview *self)
    {
        auto &event = self->m_events.at<web_event::title>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            auto title = self->page_title();
            self->m_parent->post([self, title] { self->m_events.at<web_event::title>().fire(title); });

            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_DocumentTitleChanged(Callback<TitleChanged>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_DocumentTitleChanged(token); });
    }

    template <>
    void webview::impl::setup<web_event::load>(webview *self)
    {
        auto &event = self->m_events.at<web_event::load>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_parent->post([self] { self->m_events.at<web_event::load>().fire(state::finished); });
            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_NavigationCompleted(Callback<NavigationComplete>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_NavigationCompleted(token); });
    }
} // namespace saucer
