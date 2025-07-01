#include "wk.webview.impl.hpp"

#include "scripts.hpp"
#include "request.hpp"

#include "cocoa.window.impl.hpp"

#include "wk.navigation.impl.hpp"
#include "wk.permission.impl.hpp"

#include <format>
#include <cassert>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    template <>
    void saucer::webview::impl::setup<web_event::permission>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::navigated>(webview *self)
    {
        auto &event = self->m_events.get<web_event::navigated>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer = [[Observer alloc] initWithCallback:[self]
                                                                     {
                                                                         auto url = self->url();

                                                                         if (!url.has_value())
                                                                         {
                                                                             assert(false);
                                                                             return;
                                                                         }

                                                                         self->m_events.get<web_event::navigated>().fire(url.value());
                                                                     }];

        [web_view.get() addObserver:observer.get() forKeyPath:@"URL" options:0 context:nullptr];
        event.on_clear([this, observer] { [web_view.get() removeObserver:observer.get() forKeyPath:@"URL"]; });
    }

    template <>
    void saucer::webview::impl::setup<web_event::request>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::navigate>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::favicon>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::title>(webview *self)
    {
        auto &event = self->m_events.get<web_event::title>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->m_events.get<web_event::title>().fire(self->page_title());
                              }];

        [web_view.get() addObserver:observer.get() forKeyPath:@"title" options:0 context:nullptr];
        event.on_clear([this, observer] { [web_view.get() removeObserver:observer.get() forKeyPath:@"title"]; });
    }

    template <>
    void saucer::webview::impl::setup<web_event::load>(webview *)
    {
    }

    std::string webview::impl::inject_script()
    {
        static constexpr auto internal = R"js(
            message: async (message) =>
            {
                window.webkit.messageHandlers.saucer.postMessage(message);
            }
        )js";

        static const auto script = std::format(scripts::webview_script, //
                                               internal,                //
                                               request::stubs());

        return script;
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.message('dom_loaded')";

    WKWebViewConfiguration *webview::impl::make_config(const options &opts)
    {
        const utils::autorelease_guard guard{};

        static auto resolve = [](id target, NSString *key) -> id
        {
            @try
            {
                return [target valueForKey:key];
            }
            @catch (...)
            {
                return nil;
            }
        };

        auto *const config = [[WKWebViewConfiguration alloc] init];
        auto *const pool   = [config processPool];

        for (const auto &flag : opts.browser_flags)
        {
            const auto delim = flag.find('=');

            if (delim == std::string::npos)
            {
                continue;
            }

            auto key   = flag.substr(0, delim);
            auto value = flag.substr(delim + 1);

            const auto data = std::format(R"json({{ "value": {} }})json", value);
            auto *const str = [NSString stringWithUTF8String:data.c_str()];

            NSError *error{};

            NSDictionary *const dict = [NSJSONSerialization JSONObjectWithData:[str dataUsingEncoding:NSUTF8StringEncoding]
                                                                       options:0
                                                                         error:&error];

            if (error)
            {
                continue;
            }

            id target = config;

            while (key.contains('.'))
            {
                const auto delim   = key.find('.');
                const auto sub_key = key.substr(0, delim);

                key    = key.substr(delim + 1);
                target = resolve(target, [NSString stringWithUTF8String:sub_key.c_str()]);
            }

            auto *const selector = [NSString stringWithUTF8String:key.c_str()];

            if (!resolve(target, selector) && ![target respondsToSelector:NSSelectorFromString(selector)])
            {
                continue;
            }

            [target setValue:[dict objectForKey:@"value"] forKey:selector];
        }

        // https://github.com/WebKit/WebKit/blob/0ba313f0755d90540c9c97a08e481c192f78c295/Source/WebKit/UIProcess/API/Cocoa/WKProcessPool.mm#L219

        for (const auto &[name, handler] : schemes)
        {
            auto *const scheme = [NSString stringWithUTF8String:name.c_str()];

            [config setURLSchemeHandler:handler.get() forURLScheme:scheme];

#ifdef SAUCER_WEBKIT_PRIVATE
            [pool performSelector:@selector(_registerURLSchemeAsSecure:) withObject:scheme];
#endif
        }

        return config;
    }
} // namespace saucer

using namespace saucer;

@implementation MessageHandler
- (instancetype)initWithParent:(webview *)parent events:(webview::events *)events onMessage:(on_message_t)on_message
{
    self               = [super init];
    self->m_parent     = parent;
    self->m_events     = events;
    self->m_on_message = on_message;

    return self;
}

- (void)userContentController:(WKUserContentController *)controller didReceiveScriptMessage:(WKScriptMessage *)raw
{
    const utils::autorelease_guard guard{};

    const id body = raw.body;

    if (![body isKindOfClass:[NSString class]])
    {
        return;
    }

    std::string message{static_cast<NSString *>(body).UTF8String};

    auto *const thiz = m_parent;
    auto *const impl = m_parent->native<false>();

    if (message == "dom_loaded")
    {
        impl->dom_loaded = true;

        for (const auto &pending : impl->pending)
        {
            thiz->execute(pending);
        }

        impl->pending.clear();
        m_events->get<web_event::dom_ready>().fire();

        return;
    }

    (thiz->*m_on_message)(message);
}
@end

@implementation UIDelegate
- (instancetype)initWithParent:(webview *)parent events:(webview::events *)events
{
    self           = [super init];
    self->m_parent = parent;
    self->m_events = events;

    return self;
}

- (void)webView:(WKWebView *)webview
    runJavaScriptAlertPanelWithMessage:(NSString *)message
                      initiatedByFrame:(WKFrameInfo *)frame
                     completionHandler:(void (^)(void))complete
{
    const utils::autorelease_guard guard{};

    auto *const alert = [[[NSAlert alloc] init] autorelease];

    alert.messageText = message;
    alert.alertStyle  = NSAlertStyleInformational;

    [alert runModal];

    std::invoke(complete);
}

- (void)webView:(WKWebView *)webview
    runJavaScriptConfirmPanelWithMessage:(NSString *)message
                        initiatedByFrame:(WKFrameInfo *)frame
                       completionHandler:(void (^)(BOOL))complete
{
    const utils::autorelease_guard guard{};

    auto *const alert = [[[NSAlert alloc] init] autorelease];

    alert.messageText = message;
    alert.alertStyle  = NSAlertStyleInformational;

    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    std::invoke(complete, static_cast<BOOL>([alert runModal] == NSAlertFirstButtonReturn));
}

- (void)webView:(WKWebView *)webview
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin *)origin
                          initiatedByFrame:(WKFrameInfo *)frame
                                      type:(WKMediaCaptureType)type
                           decisionHandler:(void (^)(enum WKPermissionDecision))handler
{
    using permission::block_t;
    const utils::autorelease_guard guard{};

    auto request = permission::request{{
        .frame   = utils::objc_ptr<WKFrameInfo>::ref(frame),
        .handler = utils::objc_obj<block_t>::ref(handler),
        .type    = type,
    }};

    m_events->get<web_event::permission>().fire(request);
}
@end

@implementation NavigationDelegate
- (instancetype)initWithParent:(webview *)parent events:(webview::events *)events
{
    self           = [super init];
    self->m_parent = parent;
    self->m_events = events;

    return self;
}

- (void)webView:(WKWebView *)webview didStartProvisionalNavigation:(WKNavigation *)navigation
{
    m_parent->native<false>()->dom_loaded = false;
    m_events->get<web_event::load>().fire(state::started);
}

- (void)webView:(WKWebView *)webview
    decidePolicyForNavigationAction:(WKNavigationAction *)action
                    decisionHandler:(void (^)(enum WKNavigationActionPolicy))handler
{
    const utils::autorelease_guard guard{};

    auto request = navigation{{
        .action = utils::objc_ptr<WKNavigationAction>::ref(action),
    }};

    if (m_events->get<web_event::navigate>().fire(request).find(policy::block))
    {
        return std::invoke(handler, WKNavigationActionPolicyCancel);
    }

    return std::invoke(handler, WKNavigationActionPolicyAllow);
}

- (void)webView:(WKWebView *)webview didFinishNavigation:(WKNavigation *)navigation
{
    m_events->get<web_event::load>().fire(state::finished);
}
@end

@implementation SaucerView
- (instancetype)initWithParent:(webview *)parent configuration:(WKWebViewConfiguration *)configuration frame:(CGRect)frame
{
    self           = [super initWithFrame:frame configuration:configuration];
    self->m_parent = parent;

    return self;
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    const utils::autorelease_guard guard{};

    [super willOpenMenu:menu withEvent:event];

    if (!m_parent || m_parent->context_menu())
    {
        return;
    }

    [menu removeAllItems];
}

- (void)mouseDown:(NSEvent *)event
{
    const utils::autorelease_guard guard{};

    auto *const impl = m_parent->window::native<false>();

    impl->prev_click.emplace(click_event{
        .frame    = impl->window.frame,
        .position = NSEvent.mouseLocation,
    });

    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
    const utils::autorelease_guard guard{};

    m_parent->window::native<false>()->edge.reset();

    [super mouseUp:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    const utils::autorelease_guard guard{};

    [super mouseDragged:event];

    auto *const impl = m_parent->window::native<false>();

    if (!impl->edge || !impl->prev_click)
    {
        return;
    }

    auto [frame, prev] = impl->prev_click.value();
    auto edge          = impl->edge.value();

    auto current = NSEvent.mouseLocation;
    auto diff    = NSPoint{current.x - prev.x, current.y - prev.y};

    using enum window_edge;

    auto new_frame = frame;

    if (edge & right)
    {
        new_frame.size.width += diff.x;
    }
    else if (edge & left)
    {
        new_frame.origin.x += diff.x;
        new_frame.size.width -= diff.x;
    }

    if (edge & top)
    {
        new_frame.size.height += diff.y;
    }
    else if (edge & bottom)
    {
        new_frame.origin.y += diff.y;
        new_frame.size.height -= diff.y;
    }

    [impl->window setFrame:new_frame display:YES animate:NO];
}
@end
