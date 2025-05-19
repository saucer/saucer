#pragma once

#include "app.hpp"
#include "cocoa.utils.hpp"

#include <unordered_map>

#import <Cocoa/Cocoa.h>

namespace saucer
{
    struct application::impl
    {
        NSApplication *application;

      public:
        coco::future<void> future;

      public:
        std::thread::id thread;
        std::unordered_map<NSWindow *, bool> instances;

      public:
        static void init_menu();
        static screen convert(NSScreen *);
    };
} // namespace saucer
