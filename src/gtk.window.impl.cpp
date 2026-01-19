#include "gtk.window.impl.hpp"

#include "gtk.app.impl.hpp"

#include <algorithm>

namespace saucer
{
    using native = window::impl::native;
    using event  = window::event;

    std::optional<event_data> native::prev_data() const
    {
        if (!prev_click)
        {
            return std::nullopt;
        }

        const auto [event, controller] = *prev_click;
        auto *const widget             = gtk_event_controller_get_widget(controller);

        double x{}, y{};
        gdk_event_get_position(event.get(), &x, &y);

        auto *const surface = gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(widget)));
        auto *const device  = gdk_event_get_device(event.get());

        const auto button = static_cast<gint>(gdk_button_event_get_button(event.get()));
        const auto time   = gdk_event_get_time(event.get());

        return event_data{
            .device  = device,
            .surface = surface,
            .button  = button,
            .time    = time,
            .x       = x,
            .y       = y,
        };
    }

    template <>
    void native::setup<event::decorated>(impl *)
    {
    }

    template <>
    void native::setup<event::resize>(impl *self)
    {
        auto &event = self->events.get<event::resize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, impl *self)
        {
            auto [width, height] = self->size();
            self->events.get<event::resize>().fire(width, height);
        };

        const auto width  = utils::connect(window.get(), "notify::default-width", +callback, self);
        const auto height = utils::connect(window.get(), "notify::default-height", +callback, self);

        event.on_clear(
            [this, width, height]
            {
                g_signal_handler_disconnect(window.get(), width);
                g_signal_handler_disconnect(window.get(), height);
            });
    }

    template <>
    void native::setup<event::maximize>(impl *self)
    {
        auto &event = self->events.get<event::maximize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, impl *self)
        {
            self->events.get<event::maximize>().fire(self->maximized());
        };

        const auto id = utils::connect(window.get(), "notify::maximized", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(window.get(), id); });
    }

    template <>
    void native::setup<event::minimize>(impl *)
    {
    }

    template <>
    void native::setup<event::focus>(impl *self)
    {
        auto &event = self->events.get<event::focus>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, impl *self)
        {
            self->events.get<event::focus>().fire(self->focused());
        };

        const auto id = utils::connect(window.get(), "notify::is-active", +callback, self);
        event.on_clear([this, id] { g_signal_handler_disconnect(window.get(), id); });
    }

    template <>
    void native::setup<event::close>(impl *)
    {
    }

    template <>
    void native::setup<event::closed>(impl *)
    {
    }

    template <offset T>
    saucer::size native::offset(saucer::size size) const
    {
        auto *const widget = GTK_WIDGET(header);

        if (!gtk_widget_is_visible(widget))
        {
            return size;
        }

        int offset{};
        gtk_widget_measure(widget, GTK_ORIENTATION_VERTICAL, size.h, nullptr, &offset, nullptr, nullptr);

        if constexpr (T == offset::add)
        {
            size.h += offset;
        }
        else
        {
            size.h -= offset;
        }

        return size;
    }

    template saucer::size native::offset<offset::add>(saucer::size) const;
    template saucer::size native::offset<offset::sub>(saucer::size) const;

    void native::add_widget(GtkWidget *widget) const
    {
        gtk_overlay_add_overlay(content, widget);
    }

    void native::remove_widget(GtkWidget *widget) const
    {
        gtk_overlay_remove_overlay(content, widget);
    }

    void native::track(impl *self) const
    {
        auto callback = [](void *, impl *self) -> gboolean
        {
            if (self->events.get<event::close>().fire().find(policy::block))
            {
                return true;
            }

            auto *parent     = self->parent;
            auto *identifier = self->platform->window.get();

            auto *const impl = parent->native<false>()->platform.get();
            auto &instances  = impl->instances;

            instances.erase(identifier);
            self->events.get<event::closed>().fire();

            if (!impl->quit_on_last_window_closed)
            {
                return false;
            }

            if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
            {
                parent->quit();
            }

            return false;
        };

        utils::connect(window.get(), "close-request", +callback, self);
    }

    void native::start_resize(edge edge) const
    {
        GdkSurfaceEdge translated{};

        switch (edge)
        {
            using enum window::edge;

        case top:
            translated = GDK_SURFACE_EDGE_NORTH;
            break;
        case bottom:
            translated = GDK_SURFACE_EDGE_SOUTH;
            break;
        case left:
            translated = GDK_SURFACE_EDGE_WEST;
            break;
        case right:
            translated = GDK_SURFACE_EDGE_EAST;
            break;
        case top_left:
            translated = GDK_SURFACE_EDGE_NORTH_WEST;
            break;
        case top_right:
            translated = GDK_SURFACE_EDGE_NORTH_EAST;
            break;
        case bottom_left:
            translated = GDK_SURFACE_EDGE_SOUTH_WEST;
            break;
        case bottom_right:
            translated = GDK_SURFACE_EDGE_SOUTH_EAST;
            break;
        }

        const auto data = prev_data();

        if (!data.has_value())
        {
            return;
        }

        const auto [device, surface, button, time, x, y] = *data;
        gdk_toplevel_begin_resize(GDK_TOPLEVEL(surface), translated, device, button, x, y, time);
    }

    void native::update_region(impl *self) const
    {
        auto callback = [](void *, double, double, impl *self)
        {
            auto *widget  = GTK_WIDGET(self->platform->window.get());
            auto *native  = gtk_widget_get_native(widget);
            auto *surface = gtk_native_get_surface(native);

            if (!surface)
            {
                return;
            }

            gdk_surface_set_input_region(surface, self->platform->region.get());
        };

        utils::connect(motion_controller, "motion", +callback, self);
        gtk_widget_add_controller(GTK_WIDGET(self->platform->window.get()), motion_controller);
    }

    void native::update_decorations(impl *self) const
    {
        auto decorated = [](void *, GParamSpec *, impl *self)
        {
            auto &prev         = self->platform->prev_decoration;
            const auto current = self->decorations();

            if (prev.has_value() && *prev == current)
            {
                return;
            }

            prev.emplace(current);
            self->events.get<event::decorated>().fire(current);
        };

        auto fullscreen = [](void *, GParamSpec *, impl *self)
        {
            gtk_widget_set_visible(GTK_WIDGET(self->platform->header), !self->fullscreen());
        };

        utils::connect(header, "notify::visible", +decorated, self);
        utils::connect(window.get(), "notify::decorated", +decorated, self);
        utils::connect(window.get(), "notify::fullscreened", +fullscreen, self);
    }
} // namespace saucer
