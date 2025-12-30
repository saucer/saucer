#include "wk.webview.impl.hpp"

#include "error.impl.hpp"

#include "scripts.hpp"
#include "instantiate.hpp"

#include "wk.url.impl.hpp"
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
        [platform->config.get().preferences setElementFullscreenEnabled:YES];

#ifdef SAUCER_WEBKIT_PRIVATE
        auto *const settings = platform->config.get().preferences;
        [settings setValue:@YES forKey:@"mediaDevicesEnabled"];
#endif

        auto *const handler = [[[MessageHandler alloc] initWithParent:this] autorelease];
        [platform->controller addScriptMessageHandler:handler name:@"saucer"];

        static constexpr auto resize_mask = NSViewWidthSizable | NSViewMaxXMargin | NSViewHeightSizable | NSViewMaxYMargin;
        [platform->web_view.get() setAutoresizingMask:resize_mask];

        auto *const impl = window->native<false>()->platform.get();

        [platform->web_view.get() setFrame:impl->window.contentView.frame];
        [impl->window.contentView addSubview:platform->web_view.get()];

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

        set_dev_tools(false);
        window->off(window::event::closed, platform->on_closed);

        [platform->web_view.get() removeFromSuperview];
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    result<url> impl::url() const
    {
        const auto guard = utils::autorelease_guard{};
        auto *const raw  = platform->web_view.get().URL;

        if (!raw)
        {
            return err(std::errc::not_connected);
        }

        return url::impl{[raw copy]};
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

    bool impl::force_dark() const
    {
        return platform->force_dark;
    }

    color impl::background() const
    {
        const auto guard  = utils::autorelease_guard{};
        auto *const color = [platform->web_view.get().underPageBackgroundColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];

        return {
            .r = static_cast<std::uint8_t>(color.redComponent * 255.f),
            .g = static_cast<std::uint8_t>(color.greenComponent * 255.f),
            .b = static_cast<std::uint8_t>(color.blueComponent * 255.f),
            .a = static_cast<std::uint8_t>(color.alphaComponent * 255.f),
        };
    }

    bounds impl::bounds() const
    {
        const auto guard       = utils::autorelease_guard{};
        const auto [pos, size] = platform->web_view.get().frame;

        return {
            .x = static_cast<int>(pos.x),
            .y = static_cast<int>(pos.y),
            .w = static_cast<int>(size.width),
            .h = static_cast<int>(size.height),
        };
    }

    void impl::set_url(const saucer::url &url) // NOLINT(*-function-const)
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

    void impl::set_html(cstring_view html) // NOLINT(*-function-const)
    {
        [platform->web_view.get() loadHTMLString:[NSString stringWithUTF8String:html.c_str()] baseURL:nil];
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

    void impl::set_force_dark(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard     = utils::autorelease_guard{};
        platform->force_dark = enabled;

        auto *const appearance = enabled ? [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark] //
                                         : platform->appearance;

        [window->native<false>()->platform->window setAppearance:appearance];
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto guard        = utils::autorelease_guard{};
        const auto [r, g, b, a] = color;

        auto *const rgba = [NSColor colorWithSRGBRed:static_cast<CGFloat>(r) / 255.f
                                               green:static_cast<CGFloat>(g) / 255.f
                                                blue:static_cast<CGFloat>(b) / 255.f
                                               alpha:static_cast<CGFloat>(a) / 255.f];

#ifdef SAUCER_WEBKIT_PRIVATE
        using func_t = void (*)(id, SEL, BOOL);

        // https://github.com/WebKit/WebKit/blob/0e9bbd960894d8981be0d59b235924329a6c1b4f/Source/WebKit/UIProcess/API/mac/WKWebViewMac.mm#L1431

        static auto *const selector = @selector(_setDrawsBackground:);
        static auto draw_bg         = reinterpret_cast<func_t>(class_getMethodImplementation([WKWebView class], selector));

        if (!draw_bg)
        {
            return;
        }

        draw_bg(platform->web_view.get(), selector, static_cast<BOOL>(a >= 255));
#endif

        [platform->web_view.get() setUnderPageBackgroundColor:rgba];
    }

    void impl::reset_bounds() // NOLINT(*-function-const)
    {
        [platform->web_view.get() setFrame:window->native<false>()->platform->window.contentView.frame];
    }

    void impl::set_bounds(saucer::bounds bounds) // NOLINT(*-function-const)
    {
        [platform->web_view.get() setFrame:{{.x = static_cast<CGFloat>(bounds.x), .y = static_cast<CGFloat>(bounds.y)},
                                            {.width = static_cast<CGFloat>(bounds.w), .height = static_cast<CGFloat>(bounds.h)}}];
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

    void impl::execute(cstring_view code) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        [platform->web_view.get() evaluateJavaScript:[NSString stringWithUTF8String:code.c_str()] completionHandler:nil];
    }

    std::size_t impl::inject(const script &script) // NOLINT(*-function-const)
    {
        const auto id = platform->id_counter++;

        // The order of the scripts is important as they might depend on each other, as we need to clear all scripts to remove one specific
        // one, we have to re-inject all of the scripts as well - thus we have to preserve the order in which they were added. Using
        // ascending IDs ensures that the order in the `std::map` is preserved (items are sorted based on their keys).

        platform->inject(script);
        platform->scripts.emplace(id, script);

        return id;
    }

    void impl::uninject()
    {
        const utils::autorelease_guard guard{};

        auto remove = platform->scripts                                                      //
                      | std::views::filter([](auto &item) { return item.second.clearable; }) //
                      | std::views::keys                                                     //
                      | std::ranges::to<std::vector>();

        [platform->controller removeAllUserScripts];

        std::ranges::for_each(remove, [this](auto id) { platform->scripts.erase(id); });
        std::ranges::for_each(platform->scripts | std::views::values, std::bind_front(&native::inject, platform.get()));
    }

    void impl::uninject(std::size_t id) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

        if (!platform->scripts.contains(id))
        {
            return;
        }

        platform->scripts.erase(id);
        [platform->controller removeAllUserScripts];

        std::ranges::for_each(platform->scripts | std::views::values, std::bind_front(&native::inject, platform.get()));
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
