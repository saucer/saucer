#pragma once

#include "window.impl.hpp"

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

    struct window::impl::native
    {
        NSWindow *window;
        utils::objc_ptr<WindowDelegate> delegate;

      public:
        NSWindowStyleMask masks{};

      public:
        std::optional<window::edge> edge;
        std::optional<click_event> prev_click;

      public:
        template <event>
        void setup(impl *);

      public:
        void mouse_up();
        void mouse_down();

      public:
        void mouse_dragged() const;

      public:
        static void init_menu();
    };
} // namespace saucer

@interface SaucerWindow : NSWindow
{
  @public
    saucer::window::impl *me;
}
- (instancetype)initWithParent:(saucer::window::impl *)parent
                   contentRect:(NSRect)rect
                     styleMask:(NSWindowStyleMask)mask
                       backing:(NSBackingStoreType)backing
                         defer:(BOOL)defer;
@end

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
    saucer::window::impl *me;
}
- (instancetype)initWithParent:(saucer::window::impl *)parent;
@end
