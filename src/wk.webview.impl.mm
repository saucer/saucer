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
    WKWebViewConfiguration *webview::impl::config()
    {
        static WKWebViewConfiguration *instance;

        if (!instance)
        {
            instance = [[WKWebViewConfiguration alloc] init];
        }

        return instance;
    }

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
        static bool init = false;

        if (init)
        {
            return;
        }

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

                                    self->m_parent->window::m_impl->prev_click.emplace(click_event{
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

        class_replaceMethod(
            [NavigationDelegate class], @selector(webView:didStartProvisionalNavigation:),
            imp_implementationWithBlock([](MessageHandler *self, WKWebView *, WKNavigation *)
                                        { self->m_parent->m_events.at<web_event::load_started>().fire(); }),
            "v@:@");

        init = true;
    }
} // namespace saucer

@implementation Observer
- (instancetype)initWithCallback:(saucer::observer_callback_t)callback
{
    self             = [super init];
    self->m_callback = std::move(callback);

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context
{
    std::invoke(m_callback);
}
@end

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
