#include "wk.webview.impl.hpp"

#include "cocoa.utils.hpp"
#include "cocoa.window.impl.hpp"

#include "wk.navigation.impl.hpp"
#include "wk.permission.impl.hpp"

#include <format>

namespace saucer
{
    using native = webview::impl::native;
    using event  = webview::event;

    template <>
    void native::setup<event::permission>(impl *)
    {
    }

    template <>
    void native::setup<event::fullscreen>(impl *self)
    {
        auto &event = self->events.get<event::fullscreen>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer = [[Observer alloc]
            initWithCallback:[self]
            {
                const auto guard = utils::autorelease_guard{};
                const auto state = self->platform->web_view.get().fullscreenState;

                if (state == WKFullscreenStateEnteringFullscreen || state == WKFullscreenStateExitingFullscreen)
                {
                    return;
                }

                self->events.get<event::fullscreen>().fire(state == WKFullscreenStateInFullscreen).find(saucer::policy::block);
            }];

        [web_view.get() addObserver:observer.get() forKeyPath:@"fullscreenState" options:0 context:nullptr];
        event.on_clear([this, observer] { [web_view.get() removeObserver:observer.get() forKeyPath:@"fullscreenState"]; });
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

        const utils::objc_ptr<Observer> observer = [[Observer alloc] initWithCallback:[self]
                                                                     {
                                                                         auto url = self->url();

                                                                         if (!url.has_value())
                                                                         {
                                                                             assert(false);
                                                                             return;
                                                                         }

                                                                         self->events.get<event::navigated>().fire(url.value());
                                                                     }];

        [web_view.get() addObserver:observer.get() forKeyPath:@"URL" options:0 context:nullptr];
        event.on_clear([this, observer] { [web_view.get() removeObserver:observer.get() forKeyPath:@"URL"]; });
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

        const utils::objc_ptr<Observer> observer = [[Observer alloc] initWithCallback:[self]
                                                                     {
                                                                         self->events.get<event::title>().fire(self->page_title());
                                                                     }];

        [web_view.get() addObserver:observer.get() forKeyPath:@"title" options:0 context:nullptr];
        event.on_clear([this, observer] { [web_view.get() removeObserver:observer.get() forKeyPath:@"title"]; });
    }

    template <>
    void native::setup<event::load>(impl *)
    {
    }

    void native::inject(const script &script) const
    {
        using enum script::time;

        const auto guard = utils::autorelease_guard{};
        const auto time  = script.run_at == creation ? WKUserScriptInjectionTimeAtDocumentStart : WKUserScriptInjectionTimeAtDocumentEnd;

        auto *const user_script = [[[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:script.code.c_str()]
                                                          injectionTime:time
                                                       forMainFrameOnly:static_cast<BOOL>(script.no_frames)] autorelease];
        [controller addUserScript:user_script];
    }

    WKWebViewConfiguration *native::make_config(const options &opts)
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

#ifdef SAUCER_WEBKIT_PRIVATE
        id pool               = resolve(config, @"processPool");
        auto *const secure    = @selector(_registerURLSchemeAsSecure:);
        const auto can_secure = pool && [pool respondsToSelector:secure];
#endif

        for (const auto &[name, handler] : schemes)
        {
            auto *const scheme = [NSString stringWithUTF8String:name.c_str()];

#ifdef SAUCER_WEBKIT_PRIVATE
            if (can_secure)
            {
                [pool performSelector:secure withObject:scheme];
            }
#endif

            [config setURLSchemeHandler:handler.get() forURLScheme:scheme];
        }

        return config;
    }

    NSUUID *native::data_store_id(const options &opts, const std::string &id)
    {
        if (opts.storage_path)
        {
            return utils::uuid_from(opts.storage_path->string());
        }

        if (opts.persistent_cookies)
        {
            return utils::uuid_from(id);
        }

        return [[NSUUID alloc] init];
    }
} // namespace saucer

using namespace saucer;

@implementation MessageHandler
- (instancetype)initWithParent:(webview::impl *)parent
{
    self     = [super init];
    self->me = parent;

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

    if (message == "dom_loaded")
    {
        me->platform->dom_loaded = true;

        for (const auto &pending : me->platform->pending)
        {
            me->execute(pending);
        }

        me->platform->pending.clear();
        me->events.get<event::dom_ready>().fire();

        return;
    }

    me->events.get<event::message>().fire(message).find(status::handled);
}
@end

@implementation UIDelegate
- (instancetype)initWithParent:(webview::impl *)parent
{
    self     = [super init];
    self->me = parent;

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
    using permission::block_ptr;
    using permission::request;

    const utils::autorelease_guard guard{};

    auto req = std::make_shared<request>(request::impl{
        .frame   = utils::objc_ptr<WKFrameInfo>::ref(frame),
        .handler = block_ptr::ref(handler),
        .type    = type,
    });

    me->events.get<event::permission>().fire(req).find(status::handled);
}
@end

@implementation NavigationDelegate
- (instancetype)initWithParent:(webview::impl *)parent
{
    self     = [super init];
    self->me = parent;

    return self;
}

- (void)webView:(WKWebView *)webview didStartProvisionalNavigation:(WKNavigation *)navigation
{
    me->platform->dom_loaded = false;
    me->events.get<event::load>().fire(state::started);
}

- (void)webView:(WKWebView *)webview
    decidePolicyForNavigationAction:(WKNavigationAction *)action
                    decisionHandler:(void (^)(enum WKNavigationActionPolicy))handler
{
    const utils::autorelease_guard guard{};

    auto nav = navigation{{
        .action = utils::objc_ptr<WKNavigationAction>::ref(action),
    }};

    if (me->events.get<event::navigate>().fire(nav).find(policy::block))
    {
        return std::invoke(handler, WKNavigationActionPolicyCancel);
    }

    return std::invoke(handler, WKNavigationActionPolicyAllow);
}

- (void)webView:(WKWebView *)webview didFinishNavigation:(WKNavigation *)navigation
{
    me->events.get<event::load>().fire(state::finished);
}
@end

@implementation SaucerView
- (instancetype)initWithParent:(webview::impl *)parent configuration:(WKWebViewConfiguration *)configuration frame:(CGRect)frame
{
    self     = [super initWithFrame:frame configuration:configuration];
    self->me = parent;

    return self;
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    const utils::autorelease_guard guard{};

    [super willOpenMenu:menu withEvent:event];

    if (!me || me->context_menu())
    {
        return;
    }

    [menu removeAllItems];
}

- (void)mouseDown:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    me->window->native<false>()->platform->mouse_down();
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    me->window->native<false>()->platform->mouse_up();
    [super mouseUp:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    [super mouseDragged:event];
    me->window->native<false>()->platform->mouse_dragged();
}
@end
