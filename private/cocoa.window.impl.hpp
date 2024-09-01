#pragma once

#include "window.hpp"

#include <optional>
#include <functional>

#import <Cocoa/Cocoa.h>

@class WindowDelegate;

namespace saucer
{
    using app_ptr             = std::unique_ptr<NSApplication, std::function<void(NSApplication *)>>;
    using observer_callback_t = std::function<void()>;

    struct click_event
    {
        NSRect frame;
        NSPoint position;
    };

    struct window::impl
    {
        NSWindow *window;

      public:
        WindowDelegate *delegate;

      public:
        NSWindowStyleMask prev_mask;
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
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline app_ptr application;

      public:
        static void init_objc();
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

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
  @public
    saucer::window *m_parent;
}
- (instancetype)initWithParent:(saucer::window *)parent;
@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
  @public
    saucer::window *m_parent;
}
- (instancetype)initWithParent:(saucer::window *)parent;
@end
