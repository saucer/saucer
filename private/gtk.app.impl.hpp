#pragma once

#include "app.hpp"

#include <thread>
#include <unordered_map>

#include <adwaita.h>

namespace saucer
{
    struct application::impl
    {
        AdwApplication *application;

      public:
        std::thread::id thread;
        bool initialized{false};

      public:
        bool should_quit{false};
        std::unordered_map<void *, bool> instances;

      public:
        static std::string fix_id(const std::string &);
    };
} // namespace saucer
