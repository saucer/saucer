#include "wk.webview.impl.hpp"

#include "scripts.hpp"
#include "cocoa.window.impl.hpp"

#import <objc/objc-runtime.h>

#include <fmt/core.h>
#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    template <>
    void saucer::webview::impl::setup<web_event::title_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::title_changed>();

        if (!event.empty())
        {
            return;
        }

        auto *const observer =
            [[Observer alloc] initWithCallback:[self]()
                              {
                                  self->m_events.at<web_event::title_changed>().fire(self->page_title());
                              }];

        [self->m_impl->web_view addObserver:observer forKeyPath:@"title" options:0 context:nullptr];
        event.on_clear([observer, self]() { [self->m_impl->web_view removeObserver:observer forKeyPath:@"title"]; });
    }

    template <>
    void saucer::webview::impl::setup<web_event::load_finished>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::icon_changed>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::load_started>(webview *)
    {
    }

    template <>
    void saucer::webview::impl::setup<web_event::url_changed>(webview *self)
    {
        auto &event = self->m_events.at<web_event::url_changed>();

        if (!event.empty())
        {
            return;
        }

        auto *const observer = [[Observer alloc] initWithCallback:[self]()
                                                 {
                                                     self->m_events.at<web_event::url_changed>().fire(self->url());
                                                 }];

        [self->m_impl->web_view addObserver:observer forKeyPath:@"URL" options:0 context:nullptr];
        event.on_clear([observer, self]() { [self->m_impl->web_view removeObserver:observer forKeyPath:@"URL"]; });
    }

    template <>
    void saucer::webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

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
            window.webkit.messageHandlers.saucer.postMessage(message);
        }
        )js")));

        return instance.value();
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.send_message('dom_loaded')";

    void webview::impl::init_objc()
    {
        using event_func_t = void (*)(id, SEL, NSEvent *);

        auto mouse_down =
            reinterpret_cast<event_func_t>(class_getMethodImplementation([WKWebView class], @selector(mouseDown:)));

        auto mouse_up =
            reinterpret_cast<event_func_t>(class_getMethodImplementation([WKWebView class], @selector(mouseUp:)));

        auto mouse_dragged =
            reinterpret_cast<event_func_t>(class_getMethodImplementation([WKWebView class], @selector(mouseDragged:)));

        class_replaceMethod([SaucerView class], @selector(mouseDown:),
                            imp_implementationWithBlock(
                                [mouse_down](SaucerView *self, NSEvent *event)
                                {
                                    auto &impl = *self->m_parent->window::m_impl;

                                    impl.prev_click.emplace(click_event{
                                        .frame    = impl.window.frame,
                                        .position = NSEvent.mouseLocation,
                                    });

                                    mouse_down(self, @selector(mouseDown:), event);
                                }),
                            "v@:@");

        class_replaceMethod([SaucerView class], @selector(mouseUp:),
                            imp_implementationWithBlock(
                                [mouse_up](SaucerView *self, NSEvent *event)
                                {
                                    auto &impl = *self->m_parent->window::m_impl;
                                    impl.edge.reset();

                                    mouse_up(self, @selector(mouseUp:), event);
                                }),
                            "v@:@");

        class_replaceMethod([SaucerView class], @selector(mouseDragged:),
                            imp_implementationWithBlock(
                                [mouse_dragged](SaucerView *self, NSEvent *event)
                                {
                                    mouse_dragged(self, @selector(mouseDragged:), event);

                                    auto &impl = *self->m_parent->window::m_impl;

                                    if (!impl.edge || !impl.prev_click)
                                    {
                                        return;
                                    }

                                    auto [frame, prev] = impl.prev_click.value();
                                    auto edge          = impl.edge.value();

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

                                    [impl.window setFrame:new_frame display:YES animate:NO];
                                }),
                            "v@:@");

        class_replaceMethod([MessageHandler class], @selector(userContentController:didReceiveScriptMessage:),
                            imp_implementationWithBlock(
                                [](MessageHandler *self, WKUserContentController *, WKScriptMessage *message)
                                {
                                    const id body = message.body;

                                    if (![body isKindOfClass:[NSString class]])
                                    {
                                        return;
                                    }

                                    self->m_parent->on_message(static_cast<NSString *>(body).UTF8String);
                                }),
                            "v@:@");

        class_replaceMethod(
            [NavigationDelegate class], @selector(webView:didFinishNavigation:),
            imp_implementationWithBlock([](MessageHandler *self, WKWebView *, WKNavigation *)
                                        { self->m_parent->m_events.at<web_event::load_finished>().fire(); }),
            "v@:@");

        class_replaceMethod([NavigationDelegate class], @selector(webView:didStartProvisionalNavigation:),
                            imp_implementationWithBlock(
                                [](MessageHandler *self, WKWebView *, WKNavigation *)
                                {
                                    self->m_parent->m_impl->dom_loaded = false;
                                    self->m_parent->m_events.at<web_event::load_started>().fire();
                                }),
                            "v@:@");
    }

    WKWebViewConfiguration *webview::impl::make_config(const options &options)
    {
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

        for (const auto &flag : options.browser_flags)
        {
            const auto delim = flag.find('=');

            if (delim == std::string::npos)
            {
                continue;
            }

            auto key   = flag.substr(0, delim);
            auto value = flag.substr(delim + 1);

            const auto data = fmt::format(R"json({{ "value": {} }})json", value);
            auto *const str = [NSString stringWithUTF8String:data.c_str()];

            NSError *error{};

            NSDictionary *const dict =
                [NSJSONSerialization JSONObjectWithData:[str dataUsingEncoding:NSUTF8StringEncoding]
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

            if (!resolve(target, selector))
            {
                continue;
            }

            [target setValue:[dict objectForKey:@"value"] forKey:selector];
        }

        for (const auto &[name, handler] : schemes)
        {
            [config setURLSchemeHandler:handler forURLScheme:[NSString stringWithUTF8String:name.c_str()]];
        }

        return config;
    }
} // namespace saucer

@implementation MessageHandler
- (instancetype)initWithParent:(saucer::webview *)parent
{
    self           = [super init];
    self->m_parent = parent;

    return self;
}

- (void)userContentController:(nonnull WKUserContentController *)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage *)message
{
}
@end

@implementation NavigationDelegate
- (instancetype)initWithParent:(saucer::webview *)parent
{
    self           = [super init];
    self->m_parent = parent;

    return self;
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
}
@end

@implementation SaucerView
- (instancetype)initWithParent:(saucer::webview *)parent
                 configuration:(WKWebViewConfiguration *)configuration
                         frame:(CGRect)frame
{
    self           = [super initWithFrame:frame configuration:configuration];
    self->m_parent = parent;

    return self;
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    [super willOpenMenu:menu withEvent:event];

    if (m_parent->context_menu())
    {
        return;
    }

    [menu removeAllItems];
}
@end
