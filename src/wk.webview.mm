#include "wk.webview.impl.hpp"

#include "cocoa.window.impl.hpp"

#include "requests.hpp"
#include "instantiate.hpp"

#include <fmt/core.h>
#include <rebind/enum.hpp>

#import <objc/objc-runtime.h>
#import <CoreImage/CoreImage.h>
#import <CommonCrypto/CommonCrypto.h>

namespace saucer
{
    webview::webview(const preferences &prefs) : window(prefs), m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;

        std::call_once(flag,
                       []()
                       {
                           impl::init_objc();
                           register_scheme("saucer");
                       });

        m_impl->config = impl::make_config(prefs);

        NSUUID *uuid;

        if (prefs.persistent_cookies)
        {
            // https://stackoverflow.com/questions/64011825/generate-the-same-uuid-from-the-same-string

            auto id          = prefs.storage_path.empty() ? "saucer" : fmt::format("saucer.{}", prefs.storage_path.string());
            auto *const data = [NSString stringWithUTF8String:id.c_str()];

            unsigned char hash[32] = "";
            CC_SHA256(data.UTF8String, [data lengthOfBytesUsingEncoding:NSUTF8StringEncoding], hash);

            auto top = hash | std::views::drop(16) | std::ranges::to<std::vector<unsigned char>>();

            top[6] &= 0x0F;
            top[6] |= 0x50;

            top[8] &= 0x3F;
            top[8] |= 0x80;

            uuid = [[NSUUID alloc] initWithUUIDBytes:top.data()];
        }
        else
        {
            uuid = [[NSUUID alloc] init];
        }

        auto *const store = [WKWebsiteDataStore dataStoreForIdentifier:uuid];
        [m_impl->config setWebsiteDataStore:store];

        m_impl->controller = m_impl->config.userContentController;
        [m_impl->controller addScriptMessageHandler:[[MessageHandler alloc] initWithParent:this] name:@"saucer"];

        m_impl->web_view = [[SaucerView alloc] initWithParent:this configuration:m_impl->config frame:NSZeroRect];
        m_impl->delegate = [[NavigationDelegate alloc] initWithParent:this];

        [m_impl->web_view setNavigationDelegate:m_impl->delegate];

        if (!prefs.user_agent.empty())
        {
            m_impl->web_view.customUserAgent = [NSString stringWithUTF8String:prefs.user_agent.c_str()];
        }

        [window::m_impl->window setContentView:m_impl->web_view];
        m_impl->appearance = window::m_impl->window.appearance;

        window::m_impl->on_closed = [this]
        {
            set_dev_tools(false);
        };

        inject({.code = impl::inject_script(), .time = load_time::creation, .permanent = true});
        inject({.code = std::string{impl::ready_script}, .time = load_time::ready, .permanent = true});
    }

    webview::~webview()
    {
        for (const auto &event : rebind::enum_fields<web_event>)
        {
            m_events.clear(event.value);
        }

        std::invoke(window::m_impl->on_closed);
        window::m_impl->on_closed = {};

        [m_impl->controller removeAllScriptMessageHandlers];
        [m_impl->controller removeAllUserScripts];

        [m_impl->web_view setNavigationDelegate:nil];
        [window::m_impl->window setContentView:nil];

        m_impl->delegate = nil;
        m_impl->web_view = nil;
        m_impl->config   = nil;
    }

    bool webview::on_message(const std::string &message)
    {
        if (message == "dom_loaded")
        {
            m_impl->dom_loaded = true;

            for (const auto &pending : m_impl->pending)
            {
                execute(pending);
            }

            m_impl->pending.clear();
            m_events.at<web_event::dom_ready>().fire();

            return true;
        }

        auto request = requests::parse(message);

        if (!request)
        {
            return false;
        }

        if (std::holds_alternative<requests::resize>(request.value()))
        {
            const auto data = std::get<requests::resize>(request.value());
            start_resize(static_cast<window_edge>(data.edge));

            return true;
        }

        if (std::holds_alternative<requests::drag>(request.value()))
        {
            start_drag();
            return true;
        }

        return false;
    }

    icon webview::favicon() const // NOLINT(*-static)
    {
        return {};
    }

    std::string webview::page_title() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return page_title(); });
        }

        return m_impl->web_view.title.UTF8String;
    }

    bool webview::dev_tools() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return dev_tools(); });
        }

        auto *const settings = m_impl->config.preferences;
        auto *const enabled  = reinterpret_cast<NSNumber *>([settings valueForKey:@"developerExtrasEnabled"]);

        return enabled.boolValue;
    }

    std::string webview::url() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return url(); });
        }

        return m_impl->web_view.URL.absoluteString.UTF8String;
    }

    bool webview::context_menu() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return context_menu(); });
        }

        return m_impl->context_menu;
    }

    color webview::background() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return background(); });
        }

        auto *const background = m_impl->web_view.underPageBackgroundColor;
        auto *const color      = [[CIColor alloc] initWithColor:background];

        return {
            static_cast<std::uint8_t>(color.red * 255.f),
            static_cast<std::uint8_t>(color.green * 255.f),
            static_cast<std::uint8_t>(color.blue * 255.f),
            static_cast<std::uint8_t>(color.alpha * 255.f),
        };
    }

    bool webview::force_dark_mode() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return force_dark_mode(); });
        }

        return m_impl->force_dark;
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_dev_tools(enabled); });
        }

        auto *const settings = m_impl->config.preferences;
        [settings setValue:[NSNumber numberWithBool:static_cast<BOOL>(enabled)] forKey:@"developerExtrasEnabled"];

        // https://opensource.apple.com/source/WebKit2/WebKit2-7611.3.10.0.1/UIProcess/API/Cocoa/_WKInspector.mm.auto.html
        // https://opensource.apple.com/source/WebKit2/WebKit2-7611.3.10.0.1/UIProcess/API/Cocoa/WKWebView.mm.auto.html

        const id inspector = [m_impl->web_view valueForKey:@"_inspector"];

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
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_context_menu(enabled); });
        }

        m_impl->context_menu = enabled;
    }

    void webview::set_force_dark_mode(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_force_dark_mode(enabled); });
        }

        m_impl->force_dark = enabled;

        [window::m_impl->window setAppearance:enabled ? [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark] //
                                                      : m_impl->appearance];
    }

    void webview::set_background(const color &color)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, color] { return set_background(color); });
        }

        const auto [r, g, b, a] = color;
        const auto transparent  = a < 255;

        auto *const rgba = [NSColor colorWithCalibratedRed:static_cast<float>(r) / 255.f
                                                     green:static_cast<float>(g) / 255.f
                                                      blue:static_cast<float>(b) / 255.f
                                                     alpha:static_cast<float>(a) / 255.f];

        [m_impl->web_view setUnderPageBackgroundColor:rgba];
        window::m_impl->set_alpha(transparent ? 0 : 255);

        using func_t = void (*)(id, SEL, BOOL);

        static auto *const selector = @selector(_setDrawsBackground:);
        static auto draw_bg         = reinterpret_cast<func_t>(class_getMethodImplementation([WKWebView class], selector));

        if (!draw_bg)
        {
            return;
        }

        draw_bg(m_impl->web_view, selector, static_cast<BOOL>(!transparent));
    }

    void webview::set_file(const fs::path &file)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, file] { return set_file(file); });
        }

        auto *const url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];
        [m_impl->web_view loadFileURL:url allowingReadAccessToURL:url.URLByDeletingLastPathComponent];
    }

    void webview::set_url(const std::string &url)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, url] { return set_url(url); });
        }

        auto *const request = [[NSURLRequest alloc]
            initWithURL:[[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:url.c_str()]]];

        [m_impl->web_view loadRequest:request];
    }

    void webview::back()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this]() { return back(); });
        }

        [m_impl->web_view goBack];
    }

    void webview::forward()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this]() { return forward(); });
        }

        [m_impl->web_view goForward];
    }

    void webview::reload()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this]() { return reload(); });
        }

        [m_impl->web_view reload];
    }

    void webview::clear_scripts()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this]() { return clear_scripts(); });
        }

        [m_impl->controller removeAllUserScripts];

        for (const auto &script : m_impl->permanent_scripts)
        {
            inject(script);
        }
    }

    void webview::inject(const script &script)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, script]() { return inject(script); });
        }

        const auto time      = script.time == load_time::creation ? WKUserScriptInjectionTimeAtDocumentStart
                                                                  : WKUserScriptInjectionTimeAtDocumentEnd;
        const auto main_only = static_cast<BOOL>(script.frame == web_frame::top);

        auto *const user_script = [[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:script.code.c_str()]
                                                         injectionTime:time
                                                      forMainFrameOnly:main_only];

        [m_impl->controller addUserScript:user_script];

        if (!script.permanent)
        {
            return;
        }

        if (std::ranges::find(m_impl->permanent_scripts, script) != m_impl->permanent_scripts.end())
        {
            return;
        }

        m_impl->permanent_scripts.emplace_back(script);
    }

    void webview::execute(const std::string &code)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, code]() { return execute(code); });
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(code);
            return;
        }

        [m_impl->web_view evaluateJavaScript:[NSString stringWithUTF8String:code.c_str()] completionHandler:nil];
    }

    void webview::handle_scheme(const std::string &name, scheme::handler handler)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, name, handler = std::move(handler)] mutable
                            { return handle_scheme(name, std::move(handler)); });
        }

        if (!impl::schemes.contains(name))
        {
            return;
        }

        [impl::schemes[name] add_handler:std::move(handler) webview:m_impl->web_view];
    }

    void webview::remove_scheme(const std::string &name)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, name] { return remove_scheme(name); });
        }

        [impl::schemes[name] remove_handler:m_impl->web_view];
    }

    void webview::clear(web_event event)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, event]() { return clear(event); });
        }

        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, event, id] { return remove(event, id); });
        }

        m_events.remove(event, id);
    }

    template <web_event Event>
    void webview::once(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return once<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return on<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    void webview::register_scheme(const std::string &name)
    {
        if (impl::schemes.contains(name))
        {
            return;
        }

        impl::schemes.emplace(name, [[SchemeHandler alloc] init]);
    }

    INSTANTIATE_EVENTS(webview, 6, web_event)
} // namespace saucer
