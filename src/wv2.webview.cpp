#include "wv2.webview.impl.hpp"

#include "win32.error.hpp"
#include "win32.app.impl.hpp"
#include "win32.window.impl.hpp"

#include "scripts.hpp"

#include "instantiate.hpp"
#include "win32.utils.hpp"

#include <format>
#include <ranges>

#include <cassert>
#include <filesystem>

#include <shlobj.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    using impl = webview::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        auto env_options = native::env_options();
        auto flags       = opts.browser_flags;

        if (!opts.hardware_acceleration)
        {
            flags.emplace("--disable-gpu");
        }

        const auto arguments = flags                        //
                               | std::views::join_with(' ') //
                               | std::ranges::to<std::string>();

        env_options->put_AdditionalBrowserArguments(utils::widen(arguments).c_str());

        auto default_user_folder = [this](auto &&)
        {
            return native::default_user_folder(parent->native<false>()->platform->id);
        };

        const auto storage_path = opts.storage_path //
                                      .transform([]<typename T>(T &&value) { return result<fs::path>{std::forward<T>(value)}; })
                                      .value_or(err(std::errc::no_such_file_or_directory))
                                      .or_else(default_user_folder);

        if (!storage_path.has_value())
        {
            return err(storage_path);
        }

        auto environment = native::create_environment(parent, {
                                                                  .storage_path = storage_path.value(),
                                                                  .opts         = env_options.Get(),
                                                              });

        if (!environment.has_value())
        {
            return err(environment);
        }

        auto *const parent_window = window->native<false>()->platform.get();
        auto *const hwnd          = parent_window->hwnd.get();

        auto controller = native::create_controller(parent, hwnd, environment->Get());

        if (!controller.has_value())
        {
            return err(controller);
        }

        ComPtr<ICoreWebView2> raw;

        if (auto status = controller.value()->get_CoreWebView2(&raw); !SUCCEEDED(status))
        {
            return err(status);
        }

        ComPtr<ICoreWebView2_22> web_view;

        if (auto status = raw.As(&web_view); !SUCCEEDED(status))
        {
            return err(status);
        }

        platform = std::make_unique<native>();

        platform->controller = std::move(controller.value());
        platform->web_view   = std::move(web_view);
        platform->lease      = utils::lease<webview::impl *>{this};

        if (!opts.storage_path.has_value() && !opts.persistent_cookies)
        {
            platform->cleanup = storage_path.value();
            platform->web_view->get_BrowserProcessId(&platform->browser_pid);
        }

        platform->web_view->get_Settings(&platform->settings);
        platform->settings->put_IsStatusBarEnabled(false);

        if (ComPtr<ICoreWebView2Settings2> settings; opts.user_agent.has_value() && SUCCEEDED(platform->settings.As(&settings)))
        {
            settings->put_UserAgent(utils::widen(opts.user_agent.value()).c_str());
        }

        if (ComPtr<ICoreWebView2Settings3> settings; SUCCEEDED(platform->settings.As(&settings)))
        {
            settings->put_AreBrowserAcceleratorKeysEnabled(false);
        }

        set_dev_tools(false);

        auto bind = [this]<typename T, typename... Ts>(T &&func)
        {
            return std::bind_front(std::forward<T>(func), this);
        };

        platform->web_view->add_ContainsFullScreenElementChanged(Callback<Fullscreen>(bind(&native::on_fullscreen)).Get(), nullptr);
        platform->web_view->add_WebResourceRequested(Callback<ResourceRequested>(bind(&native::on_resource)).Get(), nullptr);
        platform->web_view->add_WebMessageReceived(Callback<WebMessageHandler>(bind(&native::on_message)).Get(), nullptr);
        platform->web_view->add_NewWindowRequested(Callback<NewWindowRequest>(bind(&native::on_window)).Get(), nullptr);
        platform->web_view->add_NavigationStarting(Callback<NavigationStarting>(bind(&native::on_navigation)).Get(), nullptr);
        platform->web_view->add_DOMContentLoaded(Callback<DOMLoaded>(bind(&native::on_dom)).Get(), nullptr);
        platform->web_view->add_FaviconChanged(Callback<FaviconChanged>(bind(&native::on_favicon)).Get(), nullptr);

        auto on_resize = [this, parent_window](int width, int height)
        {
            const auto bounds = platform->bounds.value_or({.x = 0, .y = 0, .w = width, .h = height});

            const auto [x, y] = parent_window->scale<mode::add>({.w = bounds.x, .h = bounds.y});
            const auto [w, h] = parent_window->scale<mode::add>({.w = bounds.w, .h = bounds.h});

            platform->controller->put_Bounds({x, y, x + w, y + h});
        };

        auto on_minimize = [this](bool minimized)
        {
            platform->controller->put_IsVisible(!minimized);
        };

        platform->on_resize   = native::bound_events--;
        platform->on_minimize = native::bound_events--;

        auto &events = window->native<false>()->events;

        events.get<window::event::resize>().update(platform->on_resize, {{
                                                                            .func      = on_resize,
                                                                            .clearable = false,
                                                                        }});

        events.get<window::event::minimize>().update(platform->on_minimize, {{
                                                                                .func      = on_minimize,
                                                                                .clearable = false,
                                                                            }});

        auto [width, height] = window->size();

        on_resize(width, height);
        on_minimize(window->minimized());

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        window->off(window::event::resize, platform->on_resize);
        window->off(window::event::minimize, platform->on_minimize);

        platform->controller->Close();

        if (!platform->cleanup.has_value())
        {
            return;
        }

        // Using `ICoreWebView2Environment5`s `add_BrowserProcessExited` sadly doesn't play that nice
        // with this architecture, as we need to have the main-loop running to receive the event,
        // but by that time, the application destructor has mostly been called already...

        utils::process_handle handle = OpenProcess(SYNCHRONIZE, false, platform->browser_pid);
        WaitForSingleObject(handle.get(), 1000);

        std::error_code ec{};
        fs::remove_all(platform->cleanup.value(), ec);
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    result<url> impl::url() const
    {
        utils::string_handle rtn;

        if (auto status = platform->web_view->get_Source(&rtn.reset()); !SUCCEEDED(status))
        {
            return err(status);
        }

        return url::parse(utils::narrow(rtn.get()));
    }

    icon impl::favicon() const
    {
        return platform->favicon;
    }

    std::string impl::page_title() const
    {
        utils::string_handle title;
        platform->web_view->get_DocumentTitle(&title.reset());

        return utils::narrow(title.get());
    }

    bool impl::dev_tools() const
    {
        BOOL rtn{false};
        platform->settings->get_AreDevToolsEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    bool impl::context_menu() const
    {
        BOOL rtn{false};
        platform->settings->get_AreDefaultContextMenusEnabled(&rtn);

        return static_cast<bool>(rtn);
    }

    bool impl::force_dark() const
    {
        ComPtr<ICoreWebView2Profile> profile;

        if (!SUCCEEDED(platform->web_view->get_Profile(&profile)))
        {
            return {};
        }

        COREWEBVIEW2_PREFERRED_COLOR_SCHEME scheme{};

        if (!SUCCEEDED(profile->get_PreferredColorScheme(&scheme)))
        {
            return {};
        }

        return scheme == COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK;
    }

    color impl::background() const
    {
        ComPtr<ICoreWebView2Controller2> controller;

        if (!SUCCEEDED(platform->controller.As(&controller)))
        {
            return {};
        }

        COREWEBVIEW2_COLOR color;
        controller->get_DefaultBackgroundColor(&color);

        return {.r = color.R, .g = color.G, .b = color.B, .a = color.A};
    }

    bounds impl::bounds() const
    {
        return unwrap_safe(platform->bounds);
    }

    void impl::set_url(const saucer::url &url) // NOLINT(*-function-const)
    {
        platform->web_view->Navigate(utils::widen(url.string()).c_str());
    }

    void impl::set_html(cstring_view html)
    {
        platform->web_view->NavigateToString(utils::widen(html).c_str());
    }

    void impl::set_dev_tools(bool enabled) // NOLINT(*-function-const)
    {
        platform->settings->put_AreDevToolsEnabled(enabled);

        if (!enabled)
        {
            return;
        }

        platform->web_view->OpenDevToolsWindow();
    }

    void impl::set_context_menu(bool enabled) // NOLINT(*-function-const)
    {
        platform->settings->put_AreDefaultContextMenusEnabled(enabled);
    }

    void impl::set_force_dark(bool enabled) // NOLINT(*-function-const)
    {
        utils::set_immersive_dark(window->native<false>()->platform->hwnd.get(), enabled);

        ComPtr<ICoreWebView2Profile> profile;

        if (!SUCCEEDED(platform->web_view->get_Profile(&profile)))
        {
            return;
        }

        profile->put_PreferredColorScheme(enabled ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK : COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO);
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        ComPtr<ICoreWebView2Controller2> controller;

        if (!SUCCEEDED(platform->controller.As(&controller)))
        {
            return;
        }

        const auto [r, g, b, a] = color;
        controller->put_DefaultBackgroundColor({.A = a, .R = r, .G = g, .B = b});
    }

    void impl::reset_bounds() // NOLINT(*-function-const)
    {
        platform->bounds.reset();
    }

    void impl::set_bounds(saucer::bounds bounds) // NOLINT(*-function-const)
    {
        platform->bounds.emplace(bounds);
    }

    void impl::back() // NOLINT(*-function-const)
    {
        platform->web_view->GoBack();
    }

    void impl::forward() // NOLINT(*-function-const)
    {
        platform->web_view->GoForward();
    }

    void impl::reload() // NOLINT(*-function-const)
    {
        platform->web_view->Reload();
    }

    void impl::execute(cstring_view code) // NOLINT(*-function-const)
    {
        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        platform->web_view->ExecuteScript(utils::widen(code).c_str(), nullptr);
    }

    std::size_t impl::inject(const script &raw)
    {
        using enum script::time;

        auto script   = wv2_script{raw};
        const auto id = platform->id_counter++;

        if (script.no_frames)
        {
            script.code = std::format(R"js(
            if (self === top)
            {{
                {}
            }}
            )js",
                                      script.code);
        }

        // As scripts that run at `ready` are iterated later on, it is important that they keep the order in which they were added.
        // Using ascending IDs ensures this, as the items of a `std::map` are sorted based on their keys.

        if (script.run_at == ready)
        {
            platform->scripts.emplace(id, std::move(script));
            return id;
        }

        auto callback = [this, id, script](auto, LPCWSTR ref) mutable
        {
            script.ref = ref;
            platform->scripts.emplace(id, std::move(script));

            return S_OK;
        };

        platform->web_view->AddScriptToExecuteOnDocumentCreated(utils::widen(script.code).c_str(),
                                                                Callback<ScriptInjected>(callback).Get());

        return id;
    }

    void impl::uninject() // NOLINT(*-function-const)
    {
        static constexpr auto uninject = static_cast<void (impl::*)(std::size_t)>(&impl::uninject);

        auto clearable = platform->scripts                                                  //
                         | std::views::filter([](auto &it) { return it.second.clearable; }) //
                         | std::views::keys                                                 //
                         | std::ranges::to<std::vector>();

        std::ranges::for_each(clearable, std::bind_front(uninject, this));
    }

    void impl::uninject(std::size_t id) // NOLINT(*-function-const)
    {
        if (!platform->scripts.contains(id))
        {
            return;
        }

        if (auto ref = platform->scripts[id].ref; ref.has_value())
        {
            platform->web_view->RemoveScriptToExecuteOnDocumentCreated(ref->c_str());
        }

        platform->scripts.erase(id);
    }

    void impl::handle_scheme(const std::string &name, scheme::resolver &&resolver) // NOLINT(*-function-const)
    {
        if (platform->schemes.contains(name))
        {
            return;
        }

        platform->schemes.emplace(name, std::move(resolver));

        const auto pattern = utils::widen(std::format("{}*", name));

        platform->web_view->AddWebResourceRequestedFilterWithRequestSourceKinds(pattern.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                                                                                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
    }

    void impl::remove_scheme(const std::string &name) // NOLINT(*-function-const)
    {
        auto it = platform->schemes.find(name);

        if (it == platform->schemes.end())
        {
            return;
        }

        const auto pattern = utils::widen(std::format("{}*", name));

        platform->web_view->RemoveWebResourceRequestedFilterWithRequestSourceKinds(pattern.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                                                                                   COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);

        platform->schemes.erase(it);
    }

    void impl::register_scheme(const std::string &name)
    {
        static std::unordered_map<std::string, ComPtr<ICoreWebView2CustomSchemeRegistration>> schemes;

        ComPtr<ICoreWebView2EnvironmentOptions4> options;

        if (!SUCCEEDED(native::env_options().As(&options)))
        {
            assert(false);
            return;
        }

        static LPCWSTR allowed_origins = L"*";
        auto scheme                    = Make<CoreWebView2CustomSchemeRegistration>(utils::widen(name).c_str());

        scheme->put_TreatAsSecure(true);
        scheme->put_HasAuthorityComponent(true);
        scheme->SetAllowedOrigins(1, &allowed_origins);

        schemes.emplace(name, std::move(scheme));

        auto mapped = std::views::transform(schemes, [](const auto &item) { return item.second.Get(); });
        std::vector<ICoreWebView2CustomSchemeRegistration *> raw{mapped.begin(), mapped.end()};

        if (SUCCEEDED(options->SetCustomSchemeRegistrations(static_cast<UINT32>(schemes.size()), raw.data())))
        {
            return;
        }

        assert(false);
    }

    std::string impl::ready_script()
    {
        return "";
    }

    std::string impl::creation_script()
    {
        static const auto script = std::format(scripts::ipc_script, R"js(
            message: async (message) =>
            {
                window.chrome.webview.postMessage(message);
            }
        )js");

        return script;
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT);
} // namespace saucer
