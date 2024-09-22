#include "app.hpp"

#include <unordered_map>

#import <Cocoa/Cocoa.h>

namespace saucer
{
    struct application::impl
    {
        NSApplication *application;

      public:
        std::thread::id thread;
        std::unordered_map<void *, bool> instances;

      public:
        static void init_menu();
    };
} // namespace saucer
