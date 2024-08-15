#pragma once

#include "window.hpp"
#include "gtk.utils.hpp"

#include <optional>
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
        g_object_ptr<AdwApplicationWindow> window;

      public:
        g_object_ptr<GtkCssProvider> style;

      public:
        GtkBox *content;
        AdwHeaderBar *header;

      public:
        std::optional<click_event> prev_click;
        [[nodiscard]] std::optional<event_data> prev_data() const;

      public:
        template <window_event>
        void setup(saucer::window *);
        void update_decorations(saucer::window *) const;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static inline bool init{false};
        static thread_local inline g_object_ptr<AdwApplication> application;
    };
} // namespace saucer
