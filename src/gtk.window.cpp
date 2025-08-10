#include "gtk.window.impl.hpp"

#include "instantiate.hpp"
#include "gtk.app.impl.hpp"

namespace saucer
{
    using impl = window::impl;

    impl::impl() = default;

    result<> impl::init_platform()
    {
        platform = std::make_unique<native>();

        auto *const application = GTK_APPLICATION(parent->native<false>()->platform->application.get());
        platform->window.reset(GTK_WINDOW(adw_application_window_new(application)));

        platform->style   = gtk_css_provider_new();
        platform->header  = ADW_HEADER_BAR(adw_header_bar_new());
        platform->content = GTK_OVERLAY(gtk_overlay_new());

        auto *const box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
        auto *const bin = ADW_BIN(adw_bin_new());

        gtk_box_append(box, GTK_WIDGET(platform->header));
        gtk_box_append(box, GTK_WIDGET(platform->content));

        gtk_widget_set_vexpand(GTK_WIDGET(bin), true);
        gtk_widget_set_hexpand(GTK_WIDGET(bin), true);

        gtk_overlay_set_child(platform->content, GTK_WIDGET(bin));
        gtk_window_set_hide_on_close(GTK_WINDOW(platform->window.get()), true);
        adw_application_window_set_content(ADW_APPLICATION_WINDOW(platform->window.get()), GTK_WIDGET(box));

        platform->class_name = std::format("background-{:d}", reinterpret_cast<std::uintptr_t>(platform->window.get()));

        auto *const display  = gdk_display_get_default();
        auto *const provider = GTK_STYLE_PROVIDER(platform->style.get());

        gtk_style_context_add_provider_for_display(display, provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
        gtk_widget_add_css_class(GTK_WIDGET(platform->window.get()), platform->class_name.c_str());

        platform->track(this);
        platform->update_decorations(this);

        set_size({800, 600});

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        // We hide-on-close. This is required to make the parent quit properly.

        events->clear(event::close);
        gtk_window_close(GTK_WINDOW(platform->window.get()));

        auto *const display  = gdk_display_get_default();
        auto *const provider = GTK_STYLE_PROVIDER(platform->style.get());

        gtk_style_context_remove_provider_for_display(display, provider);
    }

    template <window::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    bool impl::visible() const
    {
        return gtk_widget_is_visible(GTK_WIDGET(platform->window.get()));
    }

    bool impl::focused() const
    {
        return gtk_window_is_active(GTK_WINDOW(platform->window.get()));
    }

    bool impl::minimized() const // NOLINT(*-static)
    {
        return {};
    }

    bool impl::maximized() const
    {
        return gtk_window_is_maximized(GTK_WINDOW(platform->window.get()));
    }

    bool impl::resizable() const
    {
        return gtk_window_get_resizable(GTK_WINDOW(platform->window.get()));
    }

    bool impl::always_on_top() const // NOLINT(*-static)
    {
        return {};
    }

    bool impl::click_through() const
    {
        return platform->motion_controller;
    }

    std::string impl::title() const
    {
        return gtk_window_get_title(GTK_WINDOW(platform->window.get()));
    }

    color impl::background() const // NOLINT(*-static)
    {
        return {};
    }

    window::decoration impl::decorations() const
    {
        using enum decoration;

        if (!gtk_window_get_decorated(platform->window.get()))
        {
            return none;
        }

        if (!gtk_widget_get_visible(GTK_WIDGET(platform->header)))
        {
            return partial;
        }

        return full;
    }

    size impl::size() const
    {
        int width{}, height{};
        gtk_window_get_default_size(GTK_WINDOW(platform->window.get()), &width, &height);

        return {width, height};
    }

    size impl::max_size() const // NOLINT(*-static)
    {
        return {};
    }

    size impl::min_size() const
    {
        int width{}, height{};
        gtk_widget_get_size_request(GTK_WIDGET(platform->window.get()), &width, &height);

        return {width, height};
    }

    position impl::position() const // NOLINT(*-static)
    {
        return {};
    }

    std::optional<saucer::screen> impl::screen() const
    {
        auto *const widget_native = gtk_widget_get_native(GTK_WIDGET(platform->window.get()));
        auto *const surface       = gtk_native_get_surface(widget_native);

        if (!surface)
        {
            return std::nullopt;
        }

        auto *const display = gdk_display_get_default();
        auto *const monitor = gdk_display_get_monitor_at_surface(display, surface);

        if (!monitor)
        {
            return std::nullopt;
        }

        return application::impl::native::convert(monitor);
    }

    void impl::hide() const
    {
        gtk_widget_set_visible(GTK_WIDGET(platform->window.get()), false);
    }

    void impl::show() const
    {
        parent->native<false>()->platform->instances[platform->window.get()] = true;
        gtk_window_present(GTK_WINDOW(platform->window.get()));
    }

    void impl::close() const
    {
        gtk_window_close(GTK_WINDOW(platform->window.get()));
    }

    void impl::focus() const // NOLINT(*-static)
    {
    }

    void impl::start_drag() const
    {
        const auto data = platform->prev_data();

        if (!data)
        {
            return;
        }

        const auto [device, surface, button, time, x, y] = data.value();
        gdk_toplevel_begin_move(GDK_TOPLEVEL(surface), device, button, x, y, time);
    }

    void impl::start_resize(edge edge)
    {
        if (!resizable())
        {
            set_resizable(true);
            platform->prev_resizable = false;
        }

        parent->post([this, edge] { platform->start_resize(edge); });
    }

    void impl::set_minimized(bool enabled) // NOLINT(*-function-const)
    {
        if (!enabled)
        {
            gtk_window_unminimize(GTK_WINDOW(platform->window.get()));
            return;
        }

        gtk_window_minimize(GTK_WINDOW(platform->window.get()));
    }

    void impl::set_maximized(bool enabled) // NOLINT(*-function-const)
    {
        if (!enabled)
        {
            gtk_window_unmaximize(GTK_WINDOW(platform->window.get()));
            return;
        }

        gtk_window_maximize(GTK_WINDOW(platform->window.get()));
    }

    void impl::set_resizable(bool enabled) // NOLINT(*-function-const)
    {
        gtk_window_set_resizable(GTK_WINDOW(platform->window.get()), enabled);
    }

    void impl::set_always_on_top(bool) // NOLINT(*-static, *-function-const)
    {
    }

    void impl::set_click_through(bool enabled)
    {
        if (enabled && !platform->motion_controller)
        {
            platform->motion_controller = gtk_event_controller_motion_new();
            platform->region.reset(cairo_region_create());
            platform->update_region(this);

            return;
        }

        if (enabled || !platform->motion_controller)
        {
            return;
        }

        auto *const widget = GTK_WIDGET(platform->window.get());
        gtk_widget_remove_controller(widget, platform->motion_controller);

        platform->motion_controller = nullptr;
        platform->region.reset();

        gtk_widget_queue_resize(widget);
    }

    void impl::set_icon(const icon &) // NOLINT(*-static, *-function-const)
    {
    }

    void impl::set_title(const std::string &title) // NOLINT(*-function-const)
    {
        gtk_window_set_title(GTK_WINDOW(platform->window.get()), title.c_str());
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        auto [r, g, b, a] = color;
        auto format       = std::format(".{} {{ background-color: rgba({}, {}, {}, {}); }}", platform->class_name, r, g, b, a);

        gtk_css_provider_load_from_string(platform->style.get(), format.c_str());
    }

    void impl::set_decorations(decoration decoration) // NOLINT(*-function-const)
    {
        const auto decorated = decoration != decoration::none;
        const auto visible   = decoration == decoration::full;

        gtk_window_set_decorated(platform->window.get(), decorated);
        gtk_widget_set_visible(GTK_WIDGET(platform->header), visible);
    }

    void impl::set_size(saucer::size size) // NOLINT(*-function-const)
    {
        gtk_window_set_default_size(GTK_WINDOW(platform->window.get()), size.w, size.h);
    }

    void impl::set_max_size(saucer::size) // NOLINT(*-static, *-function-const)
    {
    }

    void impl::set_min_size(saucer::size size) // NOLINT(*-function-const)
    {
        gtk_widget_set_size_request(GTK_WIDGET(platform->window.get()), size.w, size.h);
    }

    void impl::set_position(saucer::position) // NOLINT(*-static, *-function-const)
    {
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT);
} // namespace saucer
