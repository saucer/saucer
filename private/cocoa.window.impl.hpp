#pragma once

#include "window.hpp"

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
}
@end

namespace saucer
{
    struct window::impl
    {
        NSWindow *window;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline NSApplication *application;
    };
} // namespace saucer
