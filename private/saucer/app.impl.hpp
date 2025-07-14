#pragma once

#include "app.hpp"

#include <thread>

namespace saucer
{
    struct application::impl
    {
        struct native;

      public:
        application::events *events;

      public:
        std::thread::id thread;
        coco::future<void> finish;

      public:
        std::unique_ptr<native> platform;

      public:
        impl();

      public:
        ~impl();

      public:
        bool init_platform(const options &);

      public:
        [[nodiscard]] std::vector<screen> screens() const;

      public:
        int run(application *, callback_t);

      public:
        void quit();
    };
} // namespace saucer
