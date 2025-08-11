#include "wk.webview.impl.hpp"

#include "scripts.hpp"
#include "instantiate.hpp"

#include "wk.uri.impl.hpp"
#include "cocoa.app.impl.hpp"
#include "cocoa.window.impl.hpp"

#include <format>
#include <algorithm>

#import <objc/objc-runtime.h>
#import <CoreImage/CoreImage.h>

namespace saucer
{
    using impl = webview::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        const utils::autorelease_guard guard{};

        platform = std::make_unique<native>();

        platform->config              = native::make_config(opts);
        platform->controller          = platform->config.get().userContentController;
        platform->web_view            = [[SaucerView alloc] initWithParent:this configuration:platform->config.get() frame:NSZeroRect];
        platform->view                = [[NSView alloc] init];
        platform->ui_delegate         = [[UIDelegate alloc] initWithParent:this];
        platform->navigation_delegate = [[NavigationDelegate alloc] initWithParent:this];

        if (opts.user_agent.has_value())
        {
            platform->web_view.get().customUserAgent = [NSString stringWithUTF8String:opts.user_agent->c_str()];
        }

        [platform->web_view.get() setUIDelegate:platform->ui_delegate.get()];
        [platform->web_view.get() setNavigationDelegate:platform->navigation_delegate.get()];

        auto *const uuid  = native::data_store_id(opts, parent->native<false>()->platform->id);
        auto *const store = [WKWebsiteDataStore dataStoreForIdentifier:uuid];

        [uuid autorelease];

        [platform->config.get() setWebsiteDataStore:store];

#ifdef SAUCER_WEBKIT_PRIVATE
        auto *const settings = platform->config.get().preferences;
        [settings setValue:@YES forKey:@"fullScreenEnabled"];
        [settings setValue:@YES forKey:@"mediaDevicesEnabled"];
#endif

        auto *const handler = [[[MessageHandler alloc] initWithParent:this] autorelease];
        [platform->controller addScriptMessageHandler:handler name:@"saucer"];

        static constexpr auto resize_mask = NSViewWidthSizable | NSViewMaxXMargin | NSViewHeightSizable | NSViewMaxYMargin;
        [platform->web_view.get() setAutoresizingMask:resize_mask];
        [platform->view.get() setAutoresizesSubviews:YES];

        [platform->web_view.get() setFrame:platform->view.get().bounds];
        [platform->view.get() addSubview:platform->web_view.get()];

        auto *const impl         = window->native<false>()->platform.get();
        impl->window.contentView = platform->view.get();

        platform->appearance = impl->window.appearance;
        platform->on_closed  = window->on<window::event::closed>({{.func = [this] { set_dev_tools(false); }, .clearable = false}});

        return {};
    }

    impl::~impl()
    {
        const utils::autorelease_guard guard{};

        for (const auto &[name, _] : native::schemes)
        {
            remove_scheme(name);
        }

        [platform->controller removeAllScriptMessageHandlers];
        [platform->controller removeAllUserScripts];

        window->off(window::event::closed, platform->on_closed);

        set_dev_tools(false);

        [platform->view.get() setSubviews:[NSArray array]];
        [window->native<false>()->platform->window setContentView:nil];
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    icon impl::favicon() const // NOLINT(*-static)
    {
        return {};
    }

    std::string impl::page_title() const
    {
        const utils::autorelease_guard guard{};
        return platform->web_view.get().title.UTF8String;
    }

    bool impl::dev_tools() const
    {
        const utils::autorelease_guard guard{};

#ifdef SAUCER_WEBKIT_PRIVATE
        auto *const settings = platform->config.get().preferences;
        auto *const enabled  = reinterpret_cast<NSNumber *>([settings valueForKey:@"developerExtrasEnabled"]);

        return enabled.boolValue;
#else
        return false;
#endif
    }

    bool impl::context_menu() const
    {
        const utils::autorelease_guard guard{};
        return platform->context_menu;
    }

    std::optional<uri> impl::url() const
    {
        const auto guard = utils::autorelease_guard{};
        auto *const url  = platform->web_view.get().URL;

        if (!url)
        {
            return std::nullopt;
        }

        return uri::impl{[url copy]};
    }

    color impl::background() const
    {
        const auto guard       = utils::autorelease_guard{};
        auto *const background = platform->web_view.get().underPageBackgroundColor;
        auto *const color      = [[[CIColor alloc] initWithColor:background] autorelease];

        return {
            .r = static_cast<std::uint8_t>(color.red * 255.f),
            .g = static_cast<std::uint8_t>(color.green * 255.f),
            .b = static_cast<std::uint8_t>(color.blue * 255.f),
            .a = static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bool impl::force_dark_mode() const
    {
        return platform->force_dark;
    }

    void impl::set_dev_tools([[maybe_unused]] bool enabled) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

#ifdef SAUCER_WEBKIT_PRIVATE
        auto *const settings = platform->config.get().preferences;
        [settings setValue:[NSNumber numberWithBool:static_cast<BOOL>(enabled)] forKey:@"developerExtrasEnabled"];

        // https://github.com/WebKit/WebKit/blob/0ba313f0755d90540c9c97a08e481c192f78c295/Source/WebKit/UIProcess/API/Cocoa/WKWebView.mm#L2694
        // https://github.com/WebKit/WebKit/blob/0ba313f0755d90540c9c97a08e481c192f78c295/Source/WebKit/UIProcess/API/Cocoa/_WKInspector.mm#L138

        const id inspector = [platform->web_view.get() valueForKey:@"_inspector"];

        if (!inspector)
        {
            return;
        }

        if (enabled)
        {
            [inspector performSelector:@selector(show)];
            return;
        }

        [inspector performSelector:@selector(close)];
#endif
    }

    void impl::set_context_menu(bool enabled) // NOLINT(*-function-const)
    {
        platform->context_menu = enabled;
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto guard        = utils::autorelease_guard{};
        const auto [r, g, b, a] = color;

        auto *const rgba = [NSColor colorWithCalibratedRed:static_cast<float>(r) / 255.f
                                                     green:static_cast<float>(g) / 255.f
                                                      blue:static_cast<float>(b) / 255.f
                                                     alpha:static_cast<float>(a) / 255.f];

        [platform->web_view.get() setUnderPageBackgroundColor:rgba];

        using func_t = void (*)(id, SEL, BOOL);

        static auto *const selector = @selector(_setDrawsBackground:);
        static auto draw_bg         = reinterpret_cast<func_t>(class_getMethodImplementation([WKWebView class], selector));

        if (!draw_bg)
        {
            return;
        }

        draw_bg(platform->web_view.get(), selector, static_cast<BOOL>(a >= 255));
    }

    void impl::set_force_dark_mode(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard     = utils::autorelease_guard{};
        platform->force_dark = enabled;

        auto *const appearance = enabled ? [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark] //
                                         : platform->appearance;

        [window->native<false>()->platform->window setAppearance:appearance];
    }

    void impl::set_url(const uri &url) // NOLINT(*-function-const)
    {
        const auto guard = utils::autorelease_guard{};
        auto *const raw  = url.native<false>()->url.get();

        if (url.scheme() == "file")
        {
            [platform->web_view.get() loadFileURL:raw allowingReadAccessToURL:raw.URLByDeletingLastPathComponent];
            return;
        }

        auto *const request = [NSURLRequest requestWithURL:raw];
        [platform->web_view.get() loadRequest:request];
    }

    void impl::back() // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->web_view.get() goBack];
    }

    void impl::forward() // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->web_view.get() goForward];
    }

    void impl::reload() // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->web_view.get() reload];
    }

    void impl::execute(const std::string &code) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        [platform->web_view.get() evaluateJavaScript:[NSString stringWithUTF8String:code.c_str()] completionHandler:nil];
    }

    std::uint64_t impl::inject(const script &script) // NOLINT(*-function-const)
    {
        using enum load_time;

        const auto guard = utils::autorelease_guard{};
        const auto time  = script.time == creation ? WKUserScriptInjectionTimeAtDocumentStart : WKUserScriptInjectionTimeAtDocumentEnd;
        const auto main_only = static_cast<BOOL>(script.frame == web_frame::top);

        auto *const user_script = [[[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:script.code.c_str()]
                                                          injectionTime:time
                                                       forMainFrameOnly:main_only] autorelease];
        [platform->controller addUserScript:user_script];

        const auto id = platform->id_counter++;
        platform->scripts.emplace(id, script);

        return id;
    }

    void impl::uninject()
    {
        const utils::autorelease_guard guard{};

        auto remaining = platform->scripts                                                   //
                         | std::views::filter([](auto &it) { return !it.second.clearable; }) //
                         | std::views::values                                                //
                         | std::ranges::to<std::vector>();

        [platform->controller removeAllUserScripts];

        platform->scripts.clear();
        std::ranges::for_each(remaining, std::bind_front(&impl::inject, this));
    }

    void impl::uninject(std::uint64_t id)
    {
        const utils::autorelease_guard guard{};

        if (!platform->scripts.contains(id))
        {
            return;
        }

        platform->scripts.erase(id);
        auto remaining = std::move(platform->scripts);

        [platform->controller removeAllUserScripts];

        platform->scripts.clear();
        std::ranges::for_each(platform->scripts | std::views::values, std::bind_front(&impl::inject, this));
    }

    void impl::handle_scheme(const std::string &name, scheme::resolver &&resolver) // NOLINT(*-function-const)
    {
        if (!native::schemes.contains(name))
        {
            return;
        }

        [native::schemes[name].get() add_callback:std::move(resolver) webview:platform->web_view.get()];
    }

    void impl::remove_scheme(const std::string &name) // NOLINT(*-function-const)
    {
        [native::schemes[name].get() del_callback:platform->web_view.get()];
    }

    void impl::register_scheme(const std::string &name)
    {
        if (native::schemes.contains(name))
        {
            return;
        }

        native::schemes.emplace(name, [[SchemeHandler alloc] init]);
    }

    std::string impl::ready_script()
    {
        return "window.saucer.internal.message('dom_loaded')";
    }

    std::string impl::creation_script()
    {
        static const auto script = std::format(scripts::ipc_script, R"js(
            message: async (message) =>
            {
                window.webkit.messageHandlers.saucer.postMessage(message);
            }
        )js");

        return script;
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT);
} // namespace saucer
