#include "gtk.window.impl.hpp"

namespace saucer
{
    bool window::impl::is_thread_safe()
    {
        return impl::application.get() != nullptr;
    }

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
        auto *const root    = GTK_WIDGET(gtk_widget_get_root(widget));

        const graphene_point_t in{static_cast<float>(x), static_cast<float>(y)};
        graphene_point_t out{};

        std::ignore = gtk_widget_compute_point(widget, root, &in, &out);

        const auto button = static_cast<gint>(gdk_button_event_get_button(event.get()));
        const auto time   = gdk_event_get_time(event.get());

        return event_data{
            .device  = device,
            .surface = surface,
            .button  = button,
            .time    = time,
            .x       = out.x,
            .y       = out.y,
        };
    }

    template <>
    void saucer::window::impl::setup<window_event::decorated>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::resize>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::resize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            auto [width, height] = self->size();
            self->m_events.at<window_event::resize>().fire(width, height);
        };

        g_signal_connect(self->m_impl->window.get(), "notify::default-width", G_CALLBACK(+callback), self);
        g_signal_connect(self->m_impl->window.get(), "notify::default-height", G_CALLBACK(+callback), self);
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::maximize>().fire(self->maximized());
        };

        g_signal_connect(self->m_impl->window.get(), "notify::maximized", G_CALLBACK(+callback), self);
    }

    template <>
    void saucer::window::impl::setup<window_event::minimize>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::focus>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::focus>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::focus>().fire(self->focused());
        };

        g_signal_connect(self->m_impl->window.get(), "notify::is-active", G_CALLBACK(+callback), self);
    }

    template <>
    void saucer::window::impl::setup<window_event::close>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::close>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](GtkWindow *, saucer::window *self)
        {
            if (self->m_events.at<window_event::close>().until(true))
            {
                return true;
            }

            self->m_events.at<window_event::closed>().fire();
            return false;
        };

        const auto id = g_signal_connect(self->m_impl->window.get(), "close-request", G_CALLBACK(+callback), self);

        auto clear = [self, id]()
        {
            if (!self->m_events.at<window_event::close>().empty())
            {
                return;
            }

            if (!self->m_events.at<window_event::closed>().empty())
            {
                return;
            }

            g_signal_handler_disconnect(self->m_impl->window.get(), id);
        };

        event.on_clear(clear);
        self->m_events.at<window_event::closed>().on_clear(clear);
    }

    template <>
    void saucer::window::impl::setup<window_event::closed>(saucer::window *self)
    {
        setup<window_event::close>(self);
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

    void window::impl::update_decorations(saucer::window *self) const
    {
        auto callback = [](GtkWindow *, GParamSpec *, saucer::window *self)
        {
            const auto decorations = self->decorations();

            gtk_widget_set_visible(GTK_WIDGET(self->m_impl->header), decorations);
            self->m_events.at<window_event::decorated>().fire(decorations);
        };

        g_signal_connect(window.get(), "notify::decorated", G_CALLBACK(+callback), self);
    }
} // namespace saucer
