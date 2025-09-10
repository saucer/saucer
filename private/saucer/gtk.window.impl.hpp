#pragma once

#include "window.impl.hpp"

#include "lease.hpp"
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

    enum class offset : std::uint8_t
    {
        add,
        sub
    };

    struct window::impl::native
    {
        utils::handle<GtkWindow *, gtk_window_destroy> window;

      public:
        std::string class_name;
        utils::g_object_ptr<GtkCssProvider> style;

      public:
        std::optional<bool> prev_resizable;
        std::optional<decoration> prev_decoration;

      public:
        GtkOverlay *content;
        AdwHeaderBar *header;

      public:
        GtkEventController *motion_controller;
        utils::handle<cairo_region_t *, cairo_region_destroy> region;

      public:
        std::optional<click_event> prev_click;
        [[nodiscard]] std::optional<event_data> prev_data() const;

      public:
        utils::lease<window::impl *> lease;

      public:
        template <event>
        void setup(impl *);

      public:
        template <offset>
        [[nodiscard]] saucer::size offset(saucer::size) const;

      public:
        void add_widget(GtkWidget *) const;
        void remove_widget(GtkWidget *) const;

      public:
        void track(impl *) const;
        void start_resize(edge) const;

      public:
        void update_region(impl *) const;
        void update_decorations(impl *) const;
    };
} // namespace saucer
