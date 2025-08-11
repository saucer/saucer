#pragma once

#include "app.impl.hpp"
#include "cocoa.utils.hpp"

#include <unordered_map>

#import <Cocoa/Cocoa.h>

namespace saucer
{
    struct application::impl::native
    {
        NSApplication *application;

      public:
        std::string id;
        bool quit_on_last_window_closed;

      public:
        std::unordered_map<NSWindow *, bool> instances;

      public:
        static void iteration();
        static screen convert(NSScreen *);

      public:
        static void init_menu();
    };
} // namespace saucer
