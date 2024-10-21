#include "cocoa.window.impl.hpp"

#include "cocoa.app.impl.hpp"

#include <algorithm>

#import <objc/objc-runtime.h>

namespace saucer
{
    void window::impl::init_objc()
    {
        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidMiniaturize:),
            imp_implementationWithBlock([](WindowDelegate *delegate, NSNotification *)
                                        { delegate->m_parent->m_events.at<window_event::minimize>().fire(true); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidDeminiaturize:),
            imp_implementationWithBlock([](WindowDelegate *delegate, NSNotification *)
                                        { delegate->m_parent->m_events.at<window_event::minimize>().fire(false); }),
            "v@:@");

        class_replaceMethod([WindowDelegate class], @selector(windowDidResize:),
                            imp_implementationWithBlock(
                                [](WindowDelegate *delegate, NSNotification *)
                                {
                                    const auto [width, height] = delegate->m_parent->size();
                                    delegate->m_parent->m_events.at<window_event::resize>().fire(width, height);
                                }),
                            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidBecomeKey:),
            imp_implementationWithBlock([](WindowDelegate *delegate, NSNotification *)
                                        { delegate->m_parent->m_events.at<window_event::focus>().fire(true); }),
            "v@:@");

        class_replaceMethod(
            [WindowDelegate class], @selector(windowDidResignKey:),
            imp_implementationWithBlock([](WindowDelegate *delegate, NSNotification *)
                                        { delegate->m_parent->m_events.at<window_event::focus>().fire(false); }),
            "v@:@");

        class_replaceMethod([WindowDelegate class], @selector(windowShouldClose:),
                            imp_implementationWithBlock(
                                [](WindowDelegate *delegate, NSNotification *)
                                {
                                    auto *self = delegate->m_parent;
                                    auto &impl = self->m_impl;

                                    if (self->m_events.at<window_event::close>().until(policy::block))
                                    {
                                        return false;
                                    }

                                    auto parent            = self->m_parent;
                                    auto *const identifier = impl->window;

                                    if (impl->on_closed)
                                    {
                                        std::invoke(impl->on_closed);
                                    }

                                    self->hide();
                                    self->m_events.at<window_event::closed>().fire();

                                    auto &instances = parent->native()->instances;
                                    instances.erase(identifier);

                                    if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
                                    {
                                        parent->quit();
                                    }

                                    return false;
                                }),
                            "v@:@");
    }

    template <>
    void saucer::window::impl::setup<window_event::decorated>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::decorated>();

        if (!event.empty())
        {
            return;
        }

        const objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->m_events.at<window_event::decorated>().fire(self->decorations());
                              }];

        [window addObserver:observer.get() forKeyPath:@"styleMask" options:0 context:nullptr];
        event.on_clear([this, observer] { [window removeObserver:observer.get() forKeyPath:@"styleMask"]; });
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        const objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->m_events.at<window_event::maximize>().fire(self->maximized());
                              }];

        [window addObserver:observer.get() forKeyPath:@"isZoomed" options:0 context:nullptr];
        event.on_clear([this, observer] { [window removeObserver:observer.get() forKeyPath:@"isZoomed"]; });
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

    void saucer::window::impl::set_alpha(std::uint8_t alpha) const
    {
        const autorelease_guard guard{};

        auto *const background = window.backgroundColor;
        auto *const color      = [background colorWithAlphaComponent:static_cast<float>(alpha) / 255.f];

        [window setBackgroundColor:color];
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

@implementation WindowDelegate
- (instancetype)initWithParent:(saucer::window *)parent
{
    self           = [super init];
    self->m_parent = parent;

    return self;
}
@end
