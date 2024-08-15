#include "cocoa.window.impl.hpp"

#import <objc/objc-runtime.h>

namespace saucer
{
    bool window::impl::is_thread_safe()
    {
        return impl::application != nullptr;
    }

    void window::impl::init_objc()
    {
        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidMiniaturize:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::minimize>().fire(true); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidDeminiaturize:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::minimize>().fire(false); }),
            "v@:@");

        class_replaceMethod([WindowDelegate class], @selector(windowWillClose:),
                            imp_implementationWithBlock(
                                [](WindowDelegate *self, NSNotification *)
                                {
                                    const auto &impl = *self->m_parent->m_impl;

                                    if (impl.on_closed)
                                    {
                                        std::invoke(impl.on_closed);
                                    }

                                    self->m_parent->m_events.at<window_event::closed>().fire();
                                }),
                            "v@:@");

        class_replaceMethod([WindowDelegate class], @selector(windowDidResize:),
                            imp_implementationWithBlock(
                                [](WindowDelegate *self, NSNotification *)
                                {
                                    const auto [width, height] = self->m_parent->size();
                                    self->m_parent->m_events.at<window_event::resize>().fire(width, height);
                                }),
                            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidBecomeKey:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::focus>().fire(true); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidResignKey:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::focus>().fire(false); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowShouldClose:),
            imp_implementationWithBlock(
                [](WindowDelegate *self, NSNotification *)
                { return !self->m_parent->m_events.at<window_event::close>().until(true).value_or(false); }),
            "v@:@");
    }
    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        auto *const observer =
            [[Observer alloc] initWithCallback:[self]()
                              {
                                  self->m_events.at<window_event::maximize>().fire(self->maximized());
                              }];

        [self->m_impl->window addObserver:observer forKeyPath:@"isZoomed" options:0 context:nullptr];
        event.on_clear([observer, self]() { [self->m_impl->window removeObserver:observer forKeyPath:@"isZoomed"]; });
    }

    template <>
    void saucer::window::impl::setup<window_event::minimize>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::closed>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::resize>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::focus>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::close>(saucer::window *)
    {
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

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    [NSApp stop:nil];
    return NO;
}

- (instancetype)initWithParent:(saucer::window *)parent
{
    self           = [super init];
    self->m_parent = parent;

    return self;
}
@end

@implementation WindowDelegate
- (instancetype)initWithParent:(saucer::window *)parent
{
    self           = [super init];
    self->m_parent = parent;

    return self;
}
@end
