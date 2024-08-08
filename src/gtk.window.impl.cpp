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
} // namespace saucer
