#include "cocoa.window.impl.hpp"

#include "cocoa.app.impl.hpp"

#include <algorithm>

namespace saucer
{
    template <>
    void saucer::window::impl::setup<window_event::decorated>(saucer::window *self)
    {
        auto &event = self->m_events.get<window_event::decorated>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->m_events.get<window_event::decorated>().fire(self->decoration());
                              }];

        [window addObserver:observer.get() forKeyPath:@"styleMask" options:0 context:nullptr];
        event.on_clear([this, observer] { [window removeObserver:observer.get() forKeyPath:@"styleMask"]; });
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.get<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        const utils::objc_ptr<Observer> observer =
            [[Observer alloc] initWithCallback:[self]
                              {
                                  self->m_events.get<window_event::maximize>().fire(self->maximized());
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
        const utils::autorelease_guard guard{};

        auto *const background = window.backgroundColor;
        auto *const color      = [background colorWithAlphaComponent:static_cast<float>(alpha) / 255.f];

        [window setBackgroundColor:color];
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
- (instancetype)initWithParent:(window *)parent events:(window::events *)events
{
    self           = [super init];
    self->m_parent = parent;
    self->m_events = events;

    return self;
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    m_events->get<window_event::minimize>().fire(true);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    m_events->get<window_event::minimize>().fire(false);
}

- (void)windowDidResize:(NSNotification *)notification
{
    const auto [width, height] = m_parent->size();
    m_events->get<window_event::resize>().fire(width, height);
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    m_events->get<window_event::focus>().fire(true);
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    m_events->get<window_event::focus>().fire(false);
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    auto *const thiz = m_parent;
    auto *const impl = thiz->native<false>();

    if (m_events->get<window_event::close>().fire().find(policy::block))
    {
        return false;
    }

    if (impl->on_closed)
    {
        std::invoke(impl->on_closed);
    }

    auto &parent           = thiz->parent();
    auto *const identifier = impl->window;

    auto *const native = parent.native<false>();
    auto &instances    = native->instances;

    thiz->hide();
    instances.erase(identifier);
    m_events->get<window_event::closed>().fire();

    if (!native->quit_on_last_window_closed)
    {
        return false;
    }

    if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
    {
        parent.quit();
    }

    return false;
}
@end
