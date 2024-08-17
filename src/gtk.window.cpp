#include "gtk.window.impl.hpp"

#include "instantiate.hpp"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <fmt/core.h>
#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    template <>
    void window::run<true>();

    template <>
    void window::run<false>();

    window::window(const options &) : m_impl(std::make_unique<impl>())
    {
        if (!impl::init)
        {
            impl::application = adw_application_new("io.saucer.saucer", G_APPLICATION_DEFAULT_FLAGS);
        }

        if (!impl::application) [[unlikely]]
        {
            throw std::runtime_error{"Construction outside of the main-thread is not permitted"};
        }

        auto callback = [](GtkApplication *, gpointer data)
        {
            auto *self        = static_cast<impl *>(data);
            auto *application = GTK_APPLICATION(impl::application.get());
            auto *window      = ADW_APPLICATION_WINDOW(adw_application_window_new(application));

            self->window  = g_object_ptr<AdwApplicationWindow>::ref(window);
            self->style   = gtk_css_provider_new();
            self->content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
            self->header  = ADW_HEADER_BAR(adw_header_bar_new());

            gtk_box_append(self->content, GTK_WIDGET(self->header));
            adw_application_window_set_content(self->window.get(), GTK_WIDGET(self->content));

            gtk_widget_add_css_class(GTK_WIDGET(self->window.get()), "window");

            auto *display  = gtk_widget_get_display(GTK_WIDGET(self->window.get()));
            auto *provider = GTK_STYLE_PROVIDER(self->style.get());

            gtk_style_context_add_provider_for_display(display, provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
        };

        if (impl::init)
        {
            std::invoke(callback, nullptr, m_impl.get());
            return;
        }

        auto id = g_signal_connect(impl::application.get(), "activate", G_CALLBACK(+callback), m_impl.get());

        while (!m_impl->window.get())
        {
            run<false>();
        }

        g_signal_handler_disconnect(impl::application.get(), id);

        //? Undecorated windows would be 0x0 otherwise

        m_impl->update_decorations(this);
        set_size(800, 600);
    }

    window::~window() = default;

    void window::dispatch(callback_t callback) const // NOLINT
    {
        auto once = [](callback_t *data)
        {
            auto callback = std::unique_ptr<callback_t>{data};
            std::invoke(*callback);
        };

        g_idle_add_once(reinterpret_cast<GSourceOnceFunc>(+once), new callback_t{std::move(callback)});
    }

    bool window::focused() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return focused(); }).get();
        }

        return gtk_window_is_active(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::minimized() const // NOLINT
    {
        return {};
    }

    bool window::maximized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return maximized(); }).get();
        }

        return gtk_window_is_maximized(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::resizable() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return resizable(); }).get();
        }

        return gtk_window_get_resizable(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::decorations() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return decorations(); }).get();
        }

        return gtk_window_get_decorated(GTK_WINDOW(m_impl->window.get()));
    }

    bool window::always_on_top() const // NOLINT
    {
        return {};
    }

    color window::background() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return background(); }).get();
        }

        return m_impl->background;
    }

    std::string window::title() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return title(); }).get();
        }

        return gtk_window_get_title(GTK_WINDOW(m_impl->window.get()));
    }

    std::pair<int, int> window::size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return size(); }).get();
        }

        int width{}, height{};
        gtk_window_get_default_size(GTK_WINDOW(m_impl->window.get()), &width, &height);

        return {width, height};
    }

    std::pair<int, int> window::max_size() const // NOLINT
    {
        return {};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return min_size(); }).get();
        }

        int width{}, height{};
        gtk_widget_get_size_request(GTK_WIDGET(m_impl->window.get()), &width, &height);

        return {width, height};
    }

    void window::hide()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return hide(); }).get();
        }

        gtk_widget_set_visible(GTK_WIDGET(m_impl->window.get()), false);
    }

    void window::show()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return show(); }).get();
        }

        gtk_window_present(GTK_WINDOW(m_impl->window.get()));
    }

    void window::close()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return close(); }).get();
        }

        gtk_window_close(GTK_WINDOW(m_impl->window.get()));
    }

    void window::focus() // NOLINT
    {
    }

    void window::start_drag()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return start_drag(); }).get();
        }

        auto data = m_impl->prev_data();

        if (!data)
        {
            return;
        }

        auto [device, surface, button, time, x, y] = data.value();
        gdk_toplevel_begin_move(GDK_TOPLEVEL(surface), device, button, x, y, time);
    }

    void window::start_resize(window_edge edge)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, edge] { return start_resize(edge); }).get();
        }

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

        auto data = m_impl->prev_data();

        if (!data)
        {
            return;
        }

        auto [device, surface, button, time, x, y] = data.value();
        gdk_toplevel_begin_resize(GDK_TOPLEVEL(surface), translated, device, button, x, y, time);
    }

    void window::set_minimized(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_minimized(enabled); }).get();
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
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_maximized(enabled); }).get();
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
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(enabled); }).get();
        }

        gtk_window_set_resizable(GTK_WINDOW(m_impl->window.get()), enabled);
    }

    void window::set_decorations(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(enabled); }).get();
        }

        gtk_window_set_decorated(GTK_WINDOW(m_impl->window.get()), enabled);
    }

    void window::set_always_on_top(bool) // NOLINT
    {
    }

    void window::set_icon(const icon &) // NOLINT
    {
    }

    void window::set_background(const color &color)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, color] { return set_background(color); }).get();
        }

        auto [r, g, b, a] = color;

        auto css = fmt::format(".window {{ background-color: rgba({}, {}, {}, {}); }}", r, g, b,
                               static_cast<float>(a) / 255.f);

        gtk_css_provider_load_from_string(m_impl->style.get(), css.c_str());

        m_impl->background = color;
    }

    void window::set_title(const std::string &title)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, title] { return set_title(title); }).get();
        }

        gtk_window_set_title(GTK_WINDOW(m_impl->window.get()), title.c_str());
    }

    void window::set_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height] { return set_size(width, height); }).get();
        }

        auto [min_width, min_height] = min_size();

        width  = std::max(width, min_width);
        height = std::max(height, min_height);

        gtk_window_set_default_size(GTK_WINDOW(m_impl->window.get()), width, height);
    }

    void window::set_max_size(int, int) // NOLINT
    {
    }

    void window::set_min_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height] { return set_min_size(width, height); }).get();
        }

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->window.get()), width, height);
    }

    void window::clear(window_event event)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, event] { return clear(event); }).get();
        }

        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, event, id] { return remove(event, id); }).get();
        }

        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::type<Event> callback)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable
                            { return once<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type<Event> callback)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return on<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    template <>
    void window::run<true>()
    {
        auto callback = [](GtkApplication *, gpointer)
        {
            // The "real" callback is registered in the constructor already, however, due to the non-async
            // workaround, we register it here again to silence GIO warnings.
        };

        g_signal_connect(impl::application.get(), "activate", G_CALLBACK(+callback), nullptr);
        g_application_run(G_APPLICATION(impl::application.get()), 0, nullptr);

        impl::application.reset();
        impl::init = false;
    }

    template <>
    void window::run<false>()
    {
        // https://github.com/GNOME/glib/blob/ce5e11aef4be46594941662a521c7f5e026cfce9/gio/gapplication.c#L2591

        auto *context = g_main_context_default();

        if (!g_main_context_acquire(context))
        {
            return;
        }

        if (!impl::init)
        {
            g_application_register(G_APPLICATION(impl::application.get()), nullptr, nullptr);
            g_application_activate(G_APPLICATION(impl::application.get()));
            impl::init = true;
        }

        g_main_context_iteration(context, false);
        g_main_context_release(context);
    }

    INSTANTIATE_EVENTS(window, 7, window_event)
} // namespace saucer
