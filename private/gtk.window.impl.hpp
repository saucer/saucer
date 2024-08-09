#pragma once

#include "window.hpp"
#include "gtk.utils.hpp"

#include <adwaita.h>

namespace saucer
{
    struct click_event
    {
        g_event_ptr event;
        GtkEventController *controller;
    };

    struct event_data
    {
        GdkDevice *device;
        GdkSurface *surface;

      public:
        int button;
        std::uint32_t time;

      public:
        double x;
        double y;
    };

    struct window::impl
    {
        AdwApplicationWindow *window;

      public:
        GtkBox *content;
        AdwHeaderBar *header;

      public:
        click_event prev_click;
        [[nodiscard]] event_data prev_data() const;

      public:
        std::optional<gulong> close_event;
        std::optional<gulong> closed_event;
        std::optional<gulong> focused_event;
        std::optional<gulong> maximize_event;
        std::optional<std::pair<gulong, gulong>> resize_event;

      public:
        template <window_event>
        void setup(saucer::window *);

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline custom_ptr<AdwApplication> application;
    };
} // namespace saucer
