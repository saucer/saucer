#include "gtk.window.impl.hpp"

#include "instantiate.hpp"
#include "gtk.app.impl.hpp"

#include <cassert>

#include <rebind/enum.hpp>

namespace saucer
{
    window::window(application *parent) : m_parent(parent), m_impl(std::make_unique<impl>())
    {
        assert(m_parent->thread_safe() && "Construction outside of the main-thread is not permitted");

        auto *const application = GTK_APPLICATION(m_parent->native<false>()->application.get());
        m_impl->window.reset(GTK_WINDOW(adw_application_window_new(application)));

        m_impl->style   = gtk_css_provider_new();
        m_impl->header  = ADW_HEADER_BAR(adw_header_bar_new());
        m_impl->content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

        gtk_box_append(m_impl->content, GTK_WIDGET(m_impl->header));
        gtk_css_provider_load_from_string(m_impl->style.get(), ".transparent { background-color: transparent; }");

        gtk_window_set_hide_on_close(GTK_WINDOW(m_impl->window.get()), true);
        adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_impl->window.get()), GTK_WIDGET(m_impl->content));

        auto *const display  = gtk_widget_get_display(GTK_WIDGET(m_impl->window.get()));
        auto *const provider = GTK_STYLE_PROVIDER(m_impl->style.get());

        gtk_style_context_add_provider_for_display(display, provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

        m_impl->track(this);
        m_impl->update_decorations(this);

        set_size(800, 600);
    }

    window::~window()
    {
        for (const auto &event : rebind::enum_values<window_event>)
        {
            m_events.clear(event);
        }

        // We hide-on-close. This is required to make the parent quit properly.
        gtk_window_close(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::visible() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return visible(); });
        }

        return gtk_widget_is_visible(GTK_WIDGET(m_impl->window.get()));
    }

    bool window::focused() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focused(); });
        }

        return gtk_window_is_active(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::minimized() const // NOLINT(*-static)
    {
        return {};
    }

    bool window::maximized() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return maximized(); });
        }

        return gtk_window_is_maximized(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::resizable() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return resizable(); });
        }

        return gtk_window_get_resizable(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::always_on_top() const // NOLINT(*-static)
    {
        return {};
    }

    bool window::click_through() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return click_through(); });
        }

        return m_impl->motion_controller;
    }

    std::string window::title() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return title(); });
        }

        return gtk_window_get_title(GTK_WINDOW(m_impl->window.get()));
    }

    window_decoration window::decoration() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return decoration(); });
        }

        if (!gtk_window_get_decorated(m_impl->window.get()))
        {
            return window_decoration::none;
        }

        if (!gtk_widget_get_visible(GTK_WIDGET(m_impl->header)))
        {
            return window_decoration::partial;
        }

        return window_decoration::full;
    }

    std::pair<int, int> window::size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return size(); });
        }

        int width{}, height{};
        gtk_window_get_default_size(GTK_WINDOW(m_impl->window.get()), &width, &height);

        return {width, height};
    }

    std::pair<int, int> window::max_size() const // NOLINT(*-static)
    {
        return {};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return min_size(); });
        }

        int width{}, height{};
        gtk_widget_get_size_request(GTK_WIDGET(m_impl->window.get()), &width, &height);

        return {width, height};
    }

    std::pair<int, int> window::position() const // NOLINT(*-static)
    {
        return {};
    }

    std::optional<saucer::screen> window::screen() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return screen(); });
        }

        auto *const native  = gtk_widget_get_native(GTK_WIDGET(m_impl->window.get()));
        auto *const surface = gtk_native_get_surface(native);

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

        return application::impl::convert(monitor);
    }

    void window::hide()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return hide(); });
        }

        gtk_widget_set_visible(GTK_WIDGET(m_impl->window.get()), false);
    }

    void window::show()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return show(); });
        }

        m_parent->native<false>()->instances[m_impl->window.get()] = true;
        gtk_window_present(GTK_WINDOW(m_impl->window.get()));
    }

    void window::close()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return close(); });
        }

        gtk_window_close(GTK_WINDOW(m_impl->window.get()));
    }

    void window::focus() // NOLINT(*-static)
    {
    }

    void window::start_drag()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return start_drag(); });
        }

        const auto data = m_impl->prev_data();

        if (!data)
        {
            return;
        }

        const auto [device, surface, button, time, x, y] = data.value();
        gdk_toplevel_begin_move(GDK_TOPLEVEL(surface), device, button, x, y, time);
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, edge] { return start_resize(edge); });
        }

        if (!resizable())
        {
            set_resizable(true);
            m_impl->prev_resizable = false;
        }

        m_parent->post([this, edge] { m_impl->start_resize(edge); });
    }

    void window::set_minimized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_minimized(enabled); });
        }

        if (!enabled)
        {
            gtk_window_unminimize(GTK_WINDOW(m_impl->window.get()));
            return;
        }

        gtk_window_minimize(GTK_WINDOW(m_impl->window.get()));
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_maximized(enabled); });
        }

        if (!enabled)
        {
            gtk_window_unmaximize(GTK_WINDOW(m_impl->window.get()));
            return;
        }

        gtk_window_maximize(GTK_WINDOW(m_impl->window.get()));
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_resizable(enabled); });
        }

        gtk_window_set_resizable(GTK_WINDOW(m_impl->window.get()), enabled);
    }

    void window::set_always_on_top(bool) // NOLINT(*-static)
    {
    }

    void window::set_click_through(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_click_through(enabled); });
        }

        if (enabled && !m_impl->motion_controller)
        {
            m_impl->motion_controller = gtk_event_controller_motion_new();
            m_impl->region.reset(cairo_region_create());
            m_impl->update_region(this);

            return;
        }

        if (enabled || !m_impl->motion_controller)
        {
            return;
        }

        auto *const widget = GTK_WIDGET(m_impl->window.get());
        gtk_widget_remove_controller(widget, m_impl->motion_controller);

        m_impl->motion_controller = nullptr;
        m_impl->region.reset();

        gtk_widget_queue_resize(widget);
    }

    void window::set_icon(const icon &) // NOLINT(*-static)
    {
    }

    void window::set_title(const std::string &title)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, title] { return set_title(title); });
        }

        gtk_window_set_title(GTK_WINDOW(m_impl->window.get()), title.c_str());
    }

    void window::set_decoration(window_decoration decoration)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, decoration] { return set_decoration(decoration); });
        }

        const auto decorated = decoration != window_decoration::none;
        const auto visible   = decoration == window_decoration::full;

        gtk_window_set_decorated(m_impl->window.get(), decorated);
        gtk_widget_set_visible(GTK_WIDGET(m_impl->header), visible);
    }

    void window::set_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_size(width, height); });
        }

        gtk_window_set_default_size(GTK_WINDOW(m_impl->window.get()), width, height);
    }

    void window::set_max_size(int, int) // NOLINT(*-static)
    {
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_min_size(width, height); });
        }

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->window.get()), width, height);
    }

    void window::set_position(int, int) // NOLINT(*-static)
    {
    }

    void window::clear(window_event event)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event] { return clear(event); });
        }

        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event, id] { return remove(event, id); });
        }

        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::event<Event>::callback callback)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, callback = std::move(callback)] mutable { return once<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        m_events.get<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::event<Event>::callback callback)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, callback = std::move(callback)] mutable { return on<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        return m_events.get<Event>().add(std::move(callback));
    }

    template <window_event Event>
    window::events::event<Event>::future window::await(events::event<Event>::future_args result)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, result = std::move(result)] mutable { return await<Event>(std::move(result)); });
        }

        m_impl->setup<Event>(this);
        return m_events.get<Event>().await(std::move(result));
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS;
} // namespace saucer
