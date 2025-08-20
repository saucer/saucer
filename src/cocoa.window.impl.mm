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

    void native::mouse_up()
    {
        const utils::autorelease_guard guard{};

        prev_click.emplace(click_event{
            .frame    = window.frame,
            .position = NSEvent.mouseLocation,
        });
    }

    void native::mouse_down()
    {
        edge.reset();
    }

    void native::mouse_dragged() const
    {
        const utils::autorelease_guard guard{};

        if (!edge.has_value() || !prev_click.has_value())
        {
            return;
        }

        auto [frame, prev] = prev_click.value();
        auto underlying    = std::to_underlying(edge.value());

        auto current   = NSEvent.mouseLocation;
        auto diff      = NSPoint{current.x - prev.x, current.y - prev.y};
        auto new_frame = frame;

        using enum window::edge;

        if (underlying & std::to_underlying(right))
        {
            new_frame.size.width += diff.x;
        }
        else if (underlying & std::to_underlying(left))
        {
            new_frame.origin.x += diff.x;
            new_frame.size.width -= diff.x;
        }

        if (underlying & std::to_underlying(top))
        {
            new_frame.size.height += diff.y;
        }
        else if (underlying & std::to_underlying(bottom))
        {
            new_frame.origin.y += diff.y;
            new_frame.size.height -= diff.y;
        }

        [window setFrame:new_frame display:YES animate:NO];
    }
} // namespace saucer

using namespace saucer;

@implementation SaucerWindow
- (instancetype)initWithParent:(saucer::window::impl *)parent
                   contentRect:(NSRect)rect
                     styleMask:(NSWindowStyleMask)mask
                       backing:(NSBackingStoreType)backing
                         defer:(BOOL)defer
{
    self     = [super initWithContentRect:rect styleMask:mask backing:backing defer:defer];
    self->me = parent;

    return self;
}

- (void)mouseDown:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    me->platform->mouse_down();
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    me->platform->mouse_up();
    [super mouseUp:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    const auto guard = utils::autorelease_guard{};

    [super mouseDragged:event];
    me->platform->mouse_dragged();
}
@end

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
