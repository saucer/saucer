#include "wv2.webview.impl.hpp"

#include "win32.app.impl.hpp"
#include "win32.window.impl.hpp"

#include "win32.error.hpp"
#include "win32.icon.impl.hpp"

#include "wv2.scheme.impl.hpp"
#include "wv2.permission.impl.hpp"
#include "wv2.navigation.impl.hpp"

#include <cassert>

#include <format>
#include <ranges>

#include <winerror.h>

#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>

namespace saucer
{
    using native = webview::impl::native;
    using event  = webview::event;

    template <>
    void native::setup<event::permission>(impl *self)
    {
        auto &event = self->events.get<event::permission>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto, ICoreWebView2PermissionRequestedEventArgs *args)
        {
            using permission::request;

            ComPtr<ICoreWebView2Deferral> deferral;

            if (auto status = args->GetDeferral(&deferral); !SUCCEEDED(status))
            {
                return status;
            }

            auto req = std::make_shared<request>(request::impl{
                .request  = args,
                .deferral = std::move(deferral),
            });

            auto fire = [req](impl *self)
            {
                self->events.get<event::permission>().fire(req).find(status::handled);
            };

            self->parent->post(utils::defer(self->platform->lease, fire));

            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_PermissionRequested(Callback<PermissionRequested>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_PermissionRequested(token); });
    }

    template <>
    void native::setup<event::fullscreen>(impl *)
    {
    }

    template <>
    void native::setup<event::dom_ready>(impl *)
    {
    }

    template <>
    void native::setup<event::navigated>(impl *self)
    {
        auto &event = self->events.get<event::navigated>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            const auto url = self->url();

            auto fire = [url](impl *self)
            {
                self->events.get<event::navigated>().fire(url);
            };

            self->parent->post(utils::defer(self->platform->lease, fire));

            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_SourceChanged(Callback<SourceChanged>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_SourceChanged(token); });
    }

    template <>
    void native::setup<event::navigate>(impl *)
    {
    }

    template <>
    void native::setup<event::message>(impl *)
    {
    }

    template <>
    void native::setup<event::request>(impl *)
    {
    }

    template <>
    void native::setup<event::favicon>(impl *)
    {
    }

    template <>
    void native::setup<event::title>(impl *self)
    {
        auto &event = self->events.get<event::title>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            auto title = self->page_title();
            self->parent->post(utils::defer(self->platform->lease, [title](impl *self) { self->events.get<event::title>().fire(title); }));

            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_DocumentTitleChanged(Callback<TitleChanged>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_DocumentTitleChanged(token); });
    }

    template <>
    void native::setup<event::load>(impl *self)
    {
        auto &event = self->events.get<event::load>();

        if (!event.empty())
        {
            return;
        }

        static constexpr auto fire = [](impl *self)
        {
            self->events.get<event::load>().fire(state::finished);
        };

        auto handler = [self](auto...)
        {
            self->parent->post(utils::defer(self->platform->lease, fire));
            return S_OK;
        };

        EventRegistrationToken token;
        web_view->add_NavigationCompleted(Callback<NavigationComplete>(handler).Get(), &token);

        event.on_clear([this, token] { web_view->remove_NavigationCompleted(token); });
    }

    void native::update_bounds(int width, int height)
    {
        auto *const parent = lease.value()->window->native<false>()->platform.get();
        const auto bb      = bounds.value_or({.x = 0, .y = 0, .w = width, .h = height});

        const auto [x, y] = parent->scale<mode::add>({.w = bb.x, .h = bb.y});
        const auto [w, h] = parent->scale<mode::add>({.w = bb.w, .h = bb.h});

        controller->put_Bounds({x, y, x + w, y + h});
    }

    ComPtr<ICoreWebView2EnvironmentOptions> native::env_options()
    {
        static auto instance = Make<CoreWebView2EnvironmentOptions>();
        return instance;
    }

    result<fs::path> native::default_user_folder(std::wstring &id)
    {
        auto hash = utils::hash({reinterpret_cast<std::uint8_t *>(id.data()), id.size()});

        if (!hash.has_value())
        {
            return err(hash);
        }

        return fs::temp_directory_path() / std::format(L"saucer-{}", *hash);
    }

    result<ComPtr<ICoreWebView2Environment>> native::create_environment(application *parent, const environment_options &options)
    {
        ComPtr<ICoreWebView2Environment> rtn{};

        auto completed = [&rtn](auto, auto *environment)
        {
            rtn = environment;
            return S_OK;
        };

        const auto &[storage_path, opts] = options;
        const auto callback              = Callback<EnvironmentCompleted>(completed);

        auto status = CreateCoreWebView2EnvironmentWithOptions(nullptr, storage_path.c_str(), opts, callback.Get());

        if (!SUCCEEDED(status))
        {
            return err(status);
        }

        while (!rtn)
        {
            parent->native<false>()->platform->iteration();
        }

        return rtn;
    }

    result<ComPtr<ICoreWebView2Controller>> native::create_controller(application *parent, HWND hwnd, ICoreWebView2Environment *env)
    {
        ComPtr<ICoreWebView2Controller> rtn{};

        auto created = [&rtn](auto, auto *controller)
        {
            rtn = controller;
            return S_OK;
        };

        const auto callback = Callback<ControllerCompleted>(created);

        if (auto status = env->CreateCoreWebView2Controller(hwnd, callback.Get()); !SUCCEEDED(status))
        {
            return err(status);
        }

        while (!rtn)
        {
            parent->native<false>()->platform->iteration();
        }

        return rtn;
    }

    HRESULT native::on_message(impl *self, ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args)
    {
        utils::string_handle raw;

        if (auto status = args->TryGetWebMessageAsString(&raw.reset()); !SUCCEEDED(status))
        {
            return status;
        }

        auto message = utils::narrow(raw.get());

        auto fire = [message = std::move(message)](impl *self)
        {
            self->events.get<event::message>().fire(message).find(status::handled);
        };

        self->parent->post(utils::defer(self->platform->lease, fire));

        return S_OK;
    }

    HRESULT native::on_resource(impl *self, ICoreWebView2 *, ICoreWebView2WebResourceRequestedEventArgs *args)
    {
        ComPtr<ICoreWebView2WebResourceRequest> request;

        if (auto status = args->get_Request(&request); !SUCCEEDED(status))
        {
            return status;
        }

        utils::string_handle raw;

        if (auto status = request->get_Uri(&raw.reset()); !SUCCEEDED(status))
        {
            return status;
        }

        auto parsed = url::parse(utils::narrow(raw.get()));

        if (!parsed.has_value())
        {
            return S_OK;
        }

        self->events.get<event::request>().fire(*parsed);

        return scheme_handler(self, {.raw = args, .request = std::move(request), .url = std::move(*parsed)});
    }

    HRESULT native::on_dom(impl *self, ICoreWebView2 *, ICoreWebView2DOMContentLoadedEventArgs *)
    {
        using enum script::time;

        for (const auto &[id, script] : self->platform->scripts)
        {
            if (script.run_at != ready)
            {
                continue;
            }

            self->execute(script.code);
        }

        self->parent->post(utils::defer(self->platform->lease, [](impl *self) { self->events.get<event::dom_ready>().fire(); }));

        return S_OK;
    }

    HRESULT native::on_navigation(impl *self, ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args)
    {
        static constexpr auto fire = [](impl *self)
        {
            self->events.get<event::load>().fire(state::started);
        };

        auto nav = navigation{navigation::impl{
            .request = args,
        }};

        if (self->events.get<event::navigate>().fire(nav).find(policy::block))
        {
            args->put_Cancel(true);
            return S_OK;
        }

        self->parent->post(utils::defer(self->platform->lease, fire));

        return S_OK;
    }

    HRESULT native::on_favicon(impl *self, ICoreWebView2 *, IUnknown *)
    {
        auto callback = [self](auto, auto *stream)
        {
            self->platform->favicon = icon{icon::impl{
                std::shared_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromStream(stream)),
            }};

            self->events.get<event::favicon>().fire(self->platform->favicon);

            return S_OK;
        };

        return self->platform->web_view->GetFavicon(COREWEBVIEW2_FAVICON_IMAGE_FORMAT_PNG, Callback<GetFavicon>(callback).Get());
    }

    HRESULT native::on_fullscreen(impl *self, ICoreWebView2 *, IUnknown *)
    {
        BOOL fullscreen{false};

        if (auto status = self->platform->web_view->get_ContainsFullScreenElement(&fullscreen); !SUCCEEDED(status))
        {
            return status;
        }

        if (!self->events.get<event::fullscreen>().fire(fullscreen).find(policy::block))
        {
            self->window->set_fullscreen(fullscreen);
        }

        return S_OK;
    }

    HRESULT native::on_window(impl *self, ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *args)
    {
        ComPtr<ICoreWebView2Deferral> deferral;

        if (auto status = args->GetDeferral(&deferral); !SUCCEEDED(status))
        {
            return status;
        }

        auto fire = [args, deferral](impl *self)
        {
            auto nav = navigation{navigation::impl{
                .request = args,
            }};

            self->events.get<event::navigate>().fire(nav).find(policy::block);
            deferral->Complete();
        };

        args->put_Handled(true);
        self->parent->post(utils::defer(self->platform->lease, fire));

        return S_OK;
    }

    HRESULT native::scheme_handler(impl *self, const scheme_options &opts)
    {
        auto &schemes = self->platform->schemes;
        auto scheme   = schemes.find(opts.url.scheme());

        if (scheme == schemes.end())
        {
            return S_OK;
        }

        ComPtr<ICoreWebView2Environment> environment;

        if (auto status = self->platform->web_view->get_Environment(&environment); !SUCCEEDED(status))
        {
            return status;
        }

        ComPtr<ICoreWebView2Deferral> deferral;

        if (auto status = opts.raw->GetDeferral(&deferral); !SUCCEEDED(status))
        {
            return status;
        }

        ComPtr<IStream> content;

        if (auto status = opts.request->get_Content(&content); !SUCCEEDED(status))
        {
            return status;
        }

        auto resolve = [environment, deferral, request = opts.raw](const scheme::response &response)
        {
            const auto *raw = reinterpret_cast<const BYTE *>(response.data.data());
            const auto size = static_cast<const UINT>(response.data.size());

            ComPtr<IStream> buffer            = SHCreateMemStream(raw, size);
            std::vector<std::wstring> headers = {std::format(L"Content-Type: {}", utils::widen(response.mime))};

            for (const auto &[name, value] : response.headers)
            {
                headers.emplace_back(std::format(L"{}: {}", utils::widen(name), utils::widen(value)));
            }

            const auto combined = headers | std::views::join_with('\n') | std::ranges::to<std::wstring>();

            ComPtr<ICoreWebView2WebResourceResponse> result;
            environment->CreateWebResourceResponse(buffer.Get(), response.status, L"OK", combined.c_str(), &result);

            request->put_Response(result.Get());
            deferral->Complete();
        };

        auto reject = [environment, deferral, request = opts.raw](const scheme::error &error)
        {
            auto status = std::to_underlying(error);
            std::wstring phrase;

            switch (error)
            {
                using enum scheme::error;

            case not_found:
                phrase = L"Not Found";
                break;
            case invalid:
                phrase = L"Bad Request";
                break;
            case denied:
                phrase = L"Unauthorized";
                break;
            default:
                break;
            }

            ComPtr<ICoreWebView2WebResourceResponse> result;
            environment->CreateWebResourceResponse(nullptr, status, L"", phrase.c_str(), &result);

            request->put_Response(result.Get());
            deferral->Complete();
        };

        auto forward = [self]<typename T>(T &&callback)
        {
            return [self, callback = std::forward<T>(callback)]<typename... Ts>(Ts &&...args) mutable
            {
                self->parent->post(utils::defer(self->platform->lease,
                                                [callback = std::forward<T>(callback), ... args = std::forward<Ts>(args)](auto *) mutable
                                                { callback(std::forward<Ts>(args)...); }));
            };
        };

        auto &resolver = scheme->second;

        auto req      = scheme::request{{.request = opts.request, .body = content}};
        auto executor = scheme::executor{forward(std::move(resolve)), forward(std::move(reject))};

        resolver(std::move(req), std::move(executor));

        return S_OK;
    }
} // namespace saucer
