#pragma once

#include "app.impl.hpp"
#include "gtk.utils.hpp"

#include <unordered_map>

#include <adwaita.h>

namespace saucer
{
    struct application::impl::native
    {
        utils::g_object_ptr<AdwApplication> application;

      public:
        int argc;
        char **argv;

      public:
        bool quit_on_last_window_closed;
        std::unordered_map<void *, bool> instances;

      public:
        static void iteration();
        static screen convert(GdkMonitor *);

      public:
        static std::string fix_id(const std::string &);
    };
} // namespace saucer
