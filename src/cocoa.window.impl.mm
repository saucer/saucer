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
            [WindowDelegate class], @selector(windowDidEnterFullScreen:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::maximize>().fire(true); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidExitFullScreen:),
            imp_implementationWithBlock([](WindowDelegate *self, NSNotification *)
                                        { self->m_parent->m_events.at<window_event::maximize>().fire(false); }),
            "v@:@");

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
                                    auto &impl = *self->m_parent->m_impl;

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
                                    auto [width, height] = self->m_parent->size();
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
} // namespace saucer

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
