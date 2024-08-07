#pragma once

#include "window.hpp"

#include <memory>
#include <functional>

#include <adwaita.h>

namespace saucer
{
    template <typename T>
    using custom_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct window::impl
    {
        AdwApplicationWindow *window;

      public:
        GtkBox *content;
        AdwHeaderBar *header;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline custom_ptr<AdwApplication> application;
    };
} // namespace saucer
