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
        std::unordered_map<void *, bool> instances;

      public:
        callback_t callback;

      public:
        static screen convert(GdkMonitor *);
        static std::string fix_id(const std::string &);
    };
} // namespace saucer
