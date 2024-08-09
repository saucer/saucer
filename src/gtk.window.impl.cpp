#include "gtk.window.impl.hpp"

namespace saucer
{
    bool window::impl::is_thread_safe()
    {
        return impl::application != nullptr;
    }

    event_data window::impl::prev_data() const
    {
        auto [event, controller] = prev_click;
        auto *widget             = gtk_event_controller_get_widget(controller);

        double x{}, y{};
        gdk_event_get_position(event.get(), &x, &y);

        auto *surface = gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(widget)));
        auto *device  = gdk_event_get_device(event.get());
        auto *root    = GTK_WIDGET(gtk_widget_get_root(widget));

        graphene_point_t in{static_cast<float>(x), static_cast<float>(y)};
        graphene_point_t out{};

        std::ignore = gtk_widget_compute_point(widget, root, &in, &out);

        auto button = static_cast<gint>(gdk_button_event_get_button(event.get()));
        auto time   = gdk_event_get_time(event.get());

        return {
            .device  = device,
            .surface = surface,
            .button  = button,
            .time    = time,
            .x       = out.x,
            .y       = out.y,
        };
    }

    template <>
    void saucer::window::impl::setup<window_event::resize>(saucer::window *self)
    {
        if (self->m_impl->resize_event)
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            auto [width, height] = self->size();
            self->m_events.at<window_event::resize>().fire(width, height);
        };

        auto width_id  = g_signal_connect(self->m_impl->window, "notify::default-width", G_CALLBACK(+callback), self);
        auto height_id = g_signal_connect(self->m_impl->window, "notify::default-height", G_CALLBACK(+callback), self);

        self->m_impl->resize_event.emplace(width_id, height_id);
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        if (self->m_impl->maximize_event)
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::maximize>().fire(self->maximized());
        };

        auto id = g_signal_connect(self->m_impl->window, "notify::maximized", G_CALLBACK(+callback), self);
        self->m_impl->maximize_event.emplace(id);
    }

    template <>
    void saucer::window::impl::setup<window_event::minimize>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::focus>(saucer::window *self)
    {
        if (self->m_impl->focused_event)
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::focus>().fire(self->focused());
        };

        auto id = g_signal_connect(self->m_impl->window, "notify::is-active", G_CALLBACK(+callback), self);
        self->m_impl->focused_event.emplace(id);
    }

    template <>
    void saucer::window::impl::setup<window_event::closed>(saucer::window *self)
    {
        if (self->m_impl->closed_event)
        {
            return;
        }

        auto callback = [](GtkWidget *, saucer::window *self)
        {
            self->m_events.at<window_event::closed>().fire();
        };

        auto id = g_signal_connect(self->m_impl->window, "destroy", G_CALLBACK(+callback), self);
        self->m_impl->closed_event.emplace(id);
    }

    template <>
    void saucer::window::impl::setup<window_event::close>(saucer::window *self)
    {
        if (self->m_impl->close_event)
        {
            return;
        }

        auto callback = [](GtkWindow *, saucer::window *self)
        {
            return self->m_events.at<window_event::close>().until(true).value_or(false);
        };

        auto id = g_signal_connect(self->m_impl->window, "close-request", G_CALLBACK(+callback), self);
        self->m_impl->close_event.emplace(id);
    }
} // namespace saucer
