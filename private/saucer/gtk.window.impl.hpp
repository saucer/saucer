#pragma once

#include "window.hpp"

#include "handle.hpp"
#include "gtk.utils.hpp"

#include <optional>

#include <adwaita.h>

namespace saucer
{
    struct click_event
    {
        utils::g_event_ptr event;
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
        utils::handle<GtkWindow *, gtk_window_destroy> window;

      public:
        utils::g_object_ptr<GtkCssProvider> style;
        std::optional<window_decoration> prev_decoration;

      public:
        GtkBox *content;
        AdwHeaderBar *header;

      public:
        GtkEventController *motion_controller;
        utils::handle<cairo_region_t *, cairo_region_destroy> region;

      public:
        std::optional<click_event> prev_click;
        [[nodiscard]] std::optional<event_data> prev_data() const;

      public:
        template <window_event>
        void setup(saucer::window *);

      public:
        void make_transparent(bool enabled) const;

      public:
        void track(saucer::window *) const;
        void update_region(saucer::window *) const;
        void update_decorations(saucer::window *) const;
    };
} // namespace saucer
