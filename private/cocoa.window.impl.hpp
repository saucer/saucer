#pragma once

#include "window.hpp"

#include <optional>
#include <functional>

#import <Cocoa/Cocoa.h>

namespace saucer
{
    using app_ptr = std::unique_ptr<NSApplication, std::function<void(NSApplication *)>>;

    struct click_event
    {
        NSRect frame;
        NSPoint position;
    };

    struct window::impl
    {
        NSWindow *window;

      public:
        std::optional<window_edge> edge;
        std::optional<click_event> prev_click;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline app_ptr application;

      public:
        static void init_objc();
    };
} // namespace saucer

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
