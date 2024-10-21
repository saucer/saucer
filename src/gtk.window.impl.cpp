#include "gtk.window.impl.hpp"

#include "gtk.app.impl.hpp"

#include <algorithm>

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
        auto &event = self->m_events.at<window_event::resize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            auto [width, height] = self->size();
            self->m_events.at<window_event::resize>().fire(width, height);
        };

        const auto width  = g_signal_connect(window.get(), "notify::default-width", G_CALLBACK(+callback), self);
        const auto height = g_signal_connect(window.get(), "notify::default-height", G_CALLBACK(+callback), self);

        event.on_clear(
            [this, width, height]()
            {
                g_signal_handler_disconnect(window.get(), width);
                g_signal_handler_disconnect(window.get(), height);
            });
    }

    template <>
    void saucer::window::impl::setup<window_event::maximize>(saucer::window *self)
    {
        auto &event = self->m_events.at<window_event::maximize>();

        if (!event.empty())
        {
            return;
        }

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::maximize>().fire(self->maximized());
        };

        const auto id = g_signal_connect(window.get(), "notify::maximized", G_CALLBACK(+callback), self);
        event.on_clear([this, id]() { g_signal_handler_disconnect(window.get(), id); });
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

        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            self->m_events.at<window_event::focus>().fire(self->focused());
        };

        const auto id = g_signal_connect(window.get(), "notify::is-active", G_CALLBACK(+callback), self);
        event.on_clear([this, id]() { g_signal_handler_disconnect(window.get(), id); });
    }

    template <>
    void saucer::window::impl::setup<window_event::close>(saucer::window *)
    {
    }

    template <>
    void saucer::window::impl::setup<window_event::closed>(saucer::window *)
    {
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
        auto callback = [](void *, saucer::window *self)
        {
            if (self->m_events.at<window_event::close>().until(policy::block))
            {
                return true;
            }

            auto parent      = self->m_parent;
            auto *identifier = self->m_impl->window.get();

            self->m_events.at<window_event::closed>().fire();

            auto &instances = parent->native()->instances;
            instances.erase(identifier);

            if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
            {
                parent->quit();
            }

            return false;
        };

        g_signal_connect(window.get(), "close-request", G_CALLBACK(+callback), self);
    }

    void window::impl::update_decorations(saucer::window *self) const
    {
        auto callback = [](void *, GParamSpec *, saucer::window *self)
        {
            const auto decorations = self->decorations();

            gtk_widget_set_visible(GTK_WIDGET(self->m_impl->header), decorations);
            self->m_events.at<window_event::decorated>().fire(decorations);
        };

        g_signal_connect(window.get(), "notify::decorated", G_CALLBACK(+callback), self);
    }
} // namespace saucer
