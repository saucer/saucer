#pragma once

#include "window.hpp"

#include <memory>
#include <functional>

#include <thread>
#include <gtk/gtk.h>

namespace saucer
{
    template <typename T>
    using custom_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct window::impl
    {
        GtkApplicationWindow *window;

      public:
        std::thread::id creation_thread;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        static thread_local inline custom_ptr<GtkApplication> application;
    };
} // namespace saucer
