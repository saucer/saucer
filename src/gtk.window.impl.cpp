#include "gtk.window.impl.hpp"

#include "gtk.app.impl.hpp"

#include <algorithm>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    std::optional<event_data> window::impl::prev_data() const
    {
        if (!prev_click)
        {
            return std::nullopt;
        }

        const auto [event, controller] = prev_click.value();
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
    void saucer::window::impl::setup<window_event::decorated>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::resize>(saucer::window *self)
    {
        auto &event = self->m_events.get<window_event::resize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            auto [width, height] = self->size();
            self->m_events.get<window_event::resize>().fire(width, height);
        };

        const auto width  = g_signal_connect(window.get(), "notify::default-width", G_CALLBACK(+callback), self);
        const auto height = g_signal_connect(window.get(), "notify::default-height", G_CALLBACK(+callback), self);

        event.on_clear(
            [this, width, height]
            {
                g_signal_handler_disconnect(window.get(), width);
                g_signal_handler_disconnect(window.get(), height);
            });
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.get<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            self->m_events.get<window_event::maximize>().fire(self->maximized());
        };

        const auto id = g_signal_connect(window.get(), "notify::maximized", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(window.get(), id); });
    }

    template <>
    void saucer::window::impl::setup<window_event::minimize>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::focus>(saucer::window *self)
    {
        auto &event = self->m_events.get<window_event::focus>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            self->m_events.get<window_event::focus>().fire(self->focused());
        };

        const auto id = g_signal_connect(window.get(), "notify::is-active", G_CALLBACK(+callback), self);
        event.on_clear([this, id] { g_signal_handler_disconnect(window.get(), id); });
    }

    template <>
    void saucer::window::impl::setup<window_event::close>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::closed>(saucer::window *)
    {
    }

    void window::impl::start_resize(window_edge edge) const
    {
        GdkSurfaceEdge translated{};

        switch (std::to_underlying(edge))
        {
            using enum window_edge;

        case std::to_underlying(top):
            translated = GDK_SURFACE_EDGE_NORTH;
            break;
        case std::to_underlying(bottom):
            translated = GDK_SURFACE_EDGE_SOUTH;
            break;
        case std::to_underlying(left):
            translated = GDK_SURFACE_EDGE_WEST;
            break;
        case std::to_underlying(right):
            translated = GDK_SURFACE_EDGE_EAST;
            break;
        case top | left:
            translated = GDK_SURFACE_EDGE_NORTH_WEST;
            break;
        case top | right:
            translated = GDK_SURFACE_EDGE_NORTH_EAST;
            break;
        case bottom | left:
            translated = GDK_SURFACE_EDGE_SOUTH_WEST;
            break;
        case bottom | right:
            translated = GDK_SURFACE_EDGE_SOUTH_EAST;
            break;
        }

        const auto data = prev_data();

        if (!data)
        {
            return;
        }

        const auto [device, surface, button, time, x, y] = data.value();
        gdk_toplevel_begin_resize(GDK_TOPLEVEL(surface), translated, device, button, x, y, time);
    }

    void window::impl::make_transparent(bool enabled) const
    {
        if (!enabled)
        {
            gtk_widget_remove_css_class(GTK_WIDGET(window.get()), "transparent");
            return;
        }

        gtk_widget_add_css_class(GTK_WIDGET(window.get()), "transparent");
    }

    void window::impl::track(saucer::window *self) const
    {
        auto callback = [](void *, saucer::window *self) -> gboolean
        {
            if (self->m_events.get<window_event::close>().fire().find(policy::block))
            {
                return true;
            }

            auto *parent     = self->m_parent;
            auto *identifier = self->m_impl->window.get();

            auto *const impl = parent->native<false>();
            auto &instances  = impl->instances;

            instances.erase(identifier);
            self->m_events.get<window_event::closed>().fire();

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

        g_signal_connect(window.get(), "close-request", G_CALLBACK(+callback), self);
    }

    void window::impl::update_region(saucer::window *self) const
    {
        auto callback = [](void *, double, double, saucer::window *self)
        {
            auto *widget  = GTK_WIDGET(self->m_impl->window.get());
            auto *native  = gtk_widget_get_native(widget);
            auto *surface = gtk_native_get_surface(native);

            if (!surface)
            {
                return;
            }

            gdk_surface_set_input_region(surface, self->m_impl->region.get());
        };

        g_signal_connect(motion_controller, "motion", G_CALLBACK(+callback), self);
        gtk_widget_add_controller(GTK_WIDGET(self->m_impl->window.get()), motion_controller);
    }

    void window::impl::update_decorations(saucer::window *self) const
    {
        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            auto &prev         = self->m_impl->prev_decoration;
            const auto current = self->decoration();

            if (prev.has_value() && prev.value() == current)
            {
                return;
            }

            prev.emplace(current);
            self->m_events.get<window_event::decorated>().fire(current);
        };

        g_signal_connect(window.get(), "notify::decorated", G_CALLBACK(+callback), self);
        g_signal_connect(header, "notify::visible", G_CALLBACK(+callback), self);
    }
} // namespace saucer
