#include "cocoa.window.impl.hpp"

#include "cocoa.app.impl.hpp"

#include <algorithm>

namespace saucer
{
    using native = window::impl::native;
    using event  = window::event;

    template <>
    void native::setup<event::decorated>(impl *self)
    {
        auto &event = self->events->get<event::decorated>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->events->get<event::decorated>().fire(self->decorations());
                              }];

        [window addObserver:observer.get() forKeyPath:@"styleMask" options:0 context:nullptr];
        event.on_clear([this, observer] { [window removeObserver:observer.get() forKeyPath:@"styleMask"]; });
    }

    template <>
    void native::setup<event::maximize>(impl *self)
    {
        auto &event = self->events->get<event::maximize>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer = [[Observer alloc] initWithCallback:[self]
                                                                     {
                                                                         self->events->get<event::maximize>().fire(self->maximized());
                                                                     }];

        [window addObserver:observer.get() forKeyPath:@"isZoomed" options:0 context:nullptr];
        event.on_clear([this, observer] { [window removeObserver:observer.get() forKeyPath:@"isZoomed"]; });
    }

    template <>
    void native::setup<event::minimize>(impl *)
    {
    }

    template <>
    void native::setup<event::closed>(impl *)
    {
    }

    template <>
    void native::setup<event::resize>(impl *)
    {
    }

    template <>
    void native::setup<event::focus>(impl *)
    {
    }

    template <>
    void native::setup<event::close>(impl *)
    {
    }
} // namespace saucer

using namespace saucer;

@implementation Observer
- (instancetype)initWithCallback:(observer_callback_t)callback
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
- (instancetype)initWithParent:(window::impl *)parent
{
    self     = [super init];
    self->me = parent;

    return self;
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    me->events->get<event::minimize>().fire(true);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    me->events->get<event::minimize>().fire(false);
}

- (void)windowDidResize:(NSNotification *)notification
{
    const auto [width, height] = me->size();
    me->events->get<event::resize>().fire(width, height);
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    me->events->get<event::focus>().fire(true);
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    me->events->get<event::focus>().fire(false);
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    if (me->events->get<event::close>().fire().find(policy::block))
    {
        return false;
    }

    auto *parent     = me->parent;
    auto *identifier = me->platform->window;

    auto *const impl = parent->native<false>()->platform.get();
    auto &instances  = impl->instances;

    me->hide();

    instances.erase(identifier);
    me->events->get<event::closed>().fire();

    if (!impl->quit_on_last_window_closed)
    {
        return false;
    }

    if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
    {
        parent->quit();
    }

    return false;
}
@end
