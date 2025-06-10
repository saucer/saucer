#pragma once

#include "app.hpp"
#include "gtk.utils.hpp"

#include <thread>
#include <unordered_map>

#include <adwaita.h>

namespace saucer
{
    struct application::impl
    {
        utils::g_object_ptr<AdwApplication> application;

      public:
        int argc;
        char **argv;

      public:
        coco::future<void> future;

      public:
        std::thread::id thread;
        bool quit_on_last_window_closed;
        std::unordered_map<void *, bool> instances;

      public:
        static void iteration();
        static screen convert(GdkMonitor *);

      public:
        static std::string fix_id(const std::string &);
    };
} // namespace saucer
