#include "wk.webview.impl.hpp"

#include "instantiate.hpp"
#include "cocoa.window.impl.hpp"

#include <algorithm>

#include <fmt/core.h>

#import <objc/objc-runtime.h>
#import <CoreImage/CoreImage.h>
#import <CommonCrypto/CommonCrypto.h>

namespace saucer
{
    webview::webview(const preferences &prefs) : window(prefs), extensible(this), m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;
        std::call_once(flag,
                       []
                       {
                           impl::init_objc();
                           register_scheme("saucer");
                       });

        const utils::autorelease_guard guard{};

        m_impl->config = impl::make_config(prefs);

        NSUUID *uuid{};

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
        [uuid autorelease];

        [m_impl->config.get() setWebsiteDataStore:store];

        auto *const settings = m_impl->config.get().preferences;
        [settings setValue:@YES forKey:@"fullScreenEnabled"];

        m_impl->controller = m_impl->config.get().userContentController;

        auto *const handler = [[[MessageHandler alloc] initWithParent:this] autorelease];
        [m_impl->controller addScriptMessageHandler:handler name:@"saucer"];

        m_impl->web_view = [[SaucerView alloc] initWithParent:this configuration:m_impl->config.get() frame:NSZeroRect];
        m_impl->delegate = [[NavigationDelegate alloc] initWithParent:this];

        [m_impl->web_view.get() setNavigationDelegate:m_impl->delegate.get()];

        if (!prefs.user_agent.empty())
        {
            m_impl->web_view.get().customUserAgent = [NSString stringWithUTF8String:prefs.user_agent.c_str()];
        }

        window::m_impl->window.contentView = m_impl->web_view.get();
        m_impl->appearance                 = window::m_impl->window.appearance;

        window::m_impl->on_closed = [this]
        {
            set_dev_tools(false);
        };

        inject({.code = impl::inject_script(), .time = load_time::creation, .permanent = true});
        inject({.code = std::string{impl::ready_script}, .time = load_time::ready, .permanent = true});
    }

    webview::~webview()
    {
        const utils::autorelease_guard guard{};

        for (const auto &[name, _] : impl::schemes)
        {
            remove_scheme(name);
        }

        std::invoke(window::m_impl->on_closed);
        window::m_impl->on_closed = {};

        [m_impl->controller removeAllScriptMessageHandlers];
        [m_impl->controller removeAllUserScripts];

        window::m_impl->window.contentView = nil;
    }

    icon webview::favicon() const // NOLINT(*-static)
    {
        return {};
    }

    std::string webview::page_title() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return page_title(); });
        }

        return m_impl->web_view.get().title.UTF8String;
    }

    bool webview::dev_tools() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return dev_tools(); });
        }

        auto *const settings = m_impl->config.get().preferences;
        auto *const enabled  = reinterpret_cast<NSNumber *>([settings valueForKey:@"developerExtrasEnabled"]);

        return enabled.boolValue;
    }

    std::string webview::url() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return url(); });
        }

        return m_impl->web_view.get().URL.absoluteString.UTF8String;
    }

    bool webview::context_menu() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return context_menu(); });
        }

        return m_impl->context_menu;
    }

    color webview::background() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return background(); });
        }

        auto *const background = m_impl->web_view.get().underPageBackgroundColor;
        auto *const color      = [[[CIColor alloc] initWithColor:background] autorelease];

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
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_dev_tools(enabled); });
        }

        auto *const settings = m_impl->config.get().preferences;
        [settings setValue:[NSNumber numberWithBool:static_cast<BOOL>(enabled)] forKey:@"developerExtrasEnabled"];

        // https://github.com/WebKit/WebKit/blob/0ba313f0755d90540c9c97a08e481c192f78c295/Source/WebKit/UIProcess/API/Cocoa/WKWebView.mm#L2694
        // https://github.com/WebKit/WebKit/blob/0ba313f0755d90540c9c97a08e481c192f78c295/Source/WebKit/UIProcess/API/Cocoa/_WKInspector.mm#L138

        const id inspector = [m_impl->web_view.get() valueForKey:@"_inspector"];

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
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_force_dark_mode(enabled); });
        }

        m_impl->force_dark = enabled;

        auto *const appearance = enabled ? [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark] //
                                         : m_impl->appearance;

        [window::m_impl->window setAppearance:appearance];
    }

    void webview::set_background(const color &color)
    {
        const utils::autorelease_guard guard{};

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

        [m_impl->web_view.get() setUnderPageBackgroundColor:rgba];

        window::m_impl->set_alpha(transparent ? 0 : 255);

        using func_t = void (*)(id, SEL, BOOL);

        static auto *const selector = @selector(_setDrawsBackground:);
        static auto draw_bg         = reinterpret_cast<func_t>(class_getMethodImplementation([WKWebView class], selector));

        if (!draw_bg)
        {
            return;
        }

        draw_bg(m_impl->web_view.get(), selector, static_cast<BOOL>(!transparent));
    }

    void webview::set_file(const fs::path &file)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, file] { return set_file(file); });
        }

        auto *const url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];
        [m_impl->web_view.get() loadFileURL:url allowingReadAccessToURL:url.URLByDeletingLastPathComponent];
    }

    void webview::set_url(const std::string &url)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, url] { return set_url(url); });
        }

        auto *const ns_url  = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
        auto *const request = [NSURLRequest requestWithURL:ns_url];

        [m_impl->web_view.get() loadRequest:request];
    }

    void webview::back()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return back(); });
        }

        [m_impl->web_view.get() goBack];
    }

    void webview::forward()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return forward(); });
        }

        [m_impl->web_view.get() goForward];
    }

    void webview::reload()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return reload(); });
        }

        [m_impl->web_view.get() reload];
    }

    void webview::clear_scripts()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return clear_scripts(); });
        }

        [m_impl->controller removeAllUserScripts];
        std::ranges::for_each(m_impl->permanent_scripts, [this](const auto &script) { inject(script); });
    }

    void webview::inject(const script &script)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, script] { return inject(script); });
        }

        const auto time      = script.time == load_time::creation ? WKUserScriptInjectionTimeAtDocumentStart
                                                                  : WKUserScriptInjectionTimeAtDocumentEnd;
        const auto main_only = static_cast<BOOL>(script.frame == web_frame::top);

        auto *const user_script = [[[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:script.code.c_str()]
                                                          injectionTime:time
                                                       forMainFrameOnly:main_only] autorelease];

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
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, code] { return execute(code); });
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(code);
            return;
        }

        [m_impl->web_view.get() evaluateJavaScript:[NSString stringWithUTF8String:code.c_str()] completionHandler:nil];
    }

    void webview::handle_scheme(const std::string &name, scheme::resolver &&resolver, launch policy)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, name, handler = std::move(resolver), policy] mutable
                            { return handle_scheme(name, std::move(handler), policy); });
        }

        if (!impl::schemes.contains(name))
        {
            return;
        }

        [impl::schemes[name].get() add_callback:{.app = m_parent.get(), .policy = policy, .resolver = std::move(resolver)}
                                        webview:m_impl->web_view.get()];
    }

    void webview::remove_scheme(const std::string &name)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, name] { return remove_scheme(name); });
        }

        [impl::schemes[name].get() del_callback:m_impl->web_view.get()];
    }

    void webview::clear(web_event event)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, event] { return clear(event); });
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
            return dispatch([this, callback = std::move(callback)] mutable //
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
            return dispatch([this, callback = std::move(callback)] mutable //
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

    SAUCER_INSTANTIATE_EVENTS(6, webview, web_event);
} // namespace saucer
