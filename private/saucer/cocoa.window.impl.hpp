#pragma once

#include "window.hpp"

#include "cocoa.utils.hpp"

#include <optional>
#include <functional>

#import <Cocoa/Cocoa.h>

@class WindowDelegate;

namespace saucer
{
    using observer_callback_t = std::function<void()>;

    static constexpr auto mask = NSWindowStyleMaskClosable         //
                                 | NSWindowStyleMaskMiniaturizable //
                                 | NSWindowStyleMaskTitled;

    struct click_event
    {
        NSRect frame;
        NSPoint position;
    };

    struct window::impl
    {
        NSWindow *window;
        utils::objc_ptr<WindowDelegate> delegate;

      public:
        NSWindowStyleMask masks{};
        std::function<void()> on_closed;

      public:
        std::optional<window_edge> edge;
        std::optional<click_event> prev_click;

      public:
        template <window_event>
        void setup(saucer::window *);

      public:
        void set_alpha(std::uint8_t alpha) const;

      public:
        static void init_menu();
    };
} // namespace saucer

@interface Observer : NSObject
{
  @public
    saucer::observer_callback_t m_callback;
}
- (instancetype)initWithCallback:(saucer::observer_callback_t)callback;
@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
  @public
    saucer::window *m_parent;
    saucer::window::events *m_events;
}
- (instancetype)initWithParent:(saucer::window *)parent events:(saucer::window::events *)events;
@end
