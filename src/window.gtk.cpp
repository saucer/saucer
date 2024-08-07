#include "window.hpp"
#include "window.gtk.impl.hpp"

#include "warnings.hpp"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    template <>
    void window::run<true>();

    template <>
    void window::run<false>();

    window::window([[maybe_unused]] const options &options) : m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;

        std::call_once(flag,
                       []()
                       {
                           auto *raw         = adw_application_new("io.saucer.saucer", G_APPLICATION_DEFAULT_FLAGS);
                           impl::application = custom_ptr<AdwApplication>(raw, [](auto *ptr) { g_object_unref(ptr); });
                       });

        if (!impl::application) [[unlikely]]
        {
            throw std::runtime_error{"Construction outside of the main-thread is not permitted"};
        }

        auto callback = [](GtkApplication *, gpointer data)
        {
            auto *thiz   = static_cast<impl *>(data);
            thiz->window = ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(impl::application.get())));

            thiz->content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
            thiz->header  = ADW_HEADER_BAR(adw_header_bar_new());

            gtk_box_append(thiz->content, GTK_WIDGET(thiz->header));
            adw_application_window_set_content(thiz->window, GTK_WIDGET(thiz->content));
        };

        auto id = g_signal_connect(impl::application.get(), "activate", G_CALLBACK(+callback), m_impl.get());

        while (!m_impl->window)
        {
            run<false>();
        }

        g_signal_handler_disconnect(impl::application.get(), id);
    }

    window::~window() = default;

    void window::dispatch(callback_t callback) const // NOLINT
    {
        auto once = [](gpointer data)
        {
            auto *callback = static_cast<callback_t *>(data);
            std::invoke(*callback);
            delete callback;
        };

        g_idle_add_once(+once, new callback_t{std::move(callback)});
    }

    bool window::focused() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return focused(); }).get();
        }

        return gtk_window_is_active(GTK_WINDOW(m_impl->window));
    }

    bool window::minimized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return minimized(); }).get();
        }

        auto *native  = gtk_widget_get_native(GTK_WIDGET(m_impl->window));
        auto *surface = gtk_native_get_surface(native);
        auto state    = gdk_toplevel_get_state(GDK_TOPLEVEL(surface));

        return state & GDK_TOPLEVEL_STATE_MINIMIZED;
    }

    bool window::maximized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return maximized(); }).get();
        }

        return gtk_window_is_maximized(GTK_WINDOW(m_impl->window));
    }

    bool window::resizable() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return maximized(); }).get();
        }

        return gtk_window_get_resizable(GTK_WINDOW(m_impl->window));
    }

    bool window::decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return decorations(); }).get();
        }

        return gtk_window_get_decorated(GTK_WINDOW(m_impl->window));
    }

    bool window::always_on_top() const // NOLINT
    {
        emit_warning<gtk_warning>();
        return {};
    }

    std::string window::title() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return title(); }).get();
        }

        return gtk_window_get_title(GTK_WINDOW(m_impl->window));
    }

    std::pair<int, int> window::size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return size(); }).get();
        }

        auto width  = gtk_widget_get_size(GTK_WIDGET(m_impl->window), GTK_ORIENTATION_HORIZONTAL);
        auto height = gtk_widget_get_size(GTK_WIDGET(m_impl->window), GTK_ORIENTATION_VERTICAL);

        return {width, height};
    }

    std::pair<int, int> window::max_size() const // NOLINT
    {
        emit_warning<gtk_warning>();
        return {};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return min_size(); }).get();
        }

        int width{};
        int height{};

        gtk_widget_get_size_request(GTK_WIDGET(m_impl->window), &width, &height);

        return {width, height};
    }

    void window::hide()
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return hide(); }).get();
        }

        gtk_widget_set_visible(GTK_WIDGET(m_impl->window), false);
    }

    void window::show()
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return show(); }).get();
        }

        gtk_window_present(GTK_WINDOW(m_impl->window));
    }

    void window::close()
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return close(); }).get();
        }

        gtk_window_close(GTK_WINDOW(m_impl->window));
    }

    void window::focus()
    {
        return show();
    }

    void window::start_drag()
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this] { return start_drag(); }).get();
        }

        auto *native  = gtk_widget_get_native(GTK_WIDGET(m_impl->window));
        auto *surface = gtk_native_get_surface(native);

        auto *display = gdk_surface_get_display(surface);
        auto *seat    = gdk_display_get_default_seat(display);
        auto *cursor  = gdk_seat_get_pointer(seat);

        gdk_toplevel_begin_move(GDK_TOPLEVEL(surface), cursor, 0, 0, 0, GDK_CURRENT_TIME);
    }

    void window::start_resize(window_edge edge)
    {
        emit_warning<gtk_buggy_warning>();
        emit_warning<gtk_wayland_warning>();

        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, edge] { return start_resize(edge); }).get();
        }

        auto *surface = gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(m_impl->window)));
        auto *display = gdk_surface_get_display(surface);
        auto *seat    = gdk_display_get_default_seat(display);
        auto *cursor  = gdk_seat_get_pointer(seat);

        int translated{};

        if (edge & window_edge::top)
        {
            translated |= GDK_SURFACE_EDGE_NORTH;
        }
        if (edge & window_edge::bottom)
        {
            translated |= GDK_SURFACE_EDGE_SOUTH;
        }
        if (edge & window_edge::left)
        {
            translated |= GDK_SURFACE_EDGE_WEST;
        }
        if (edge & window_edge::right)
        {
            translated |= GDK_SURFACE_EDGE_EAST;
        }

        gdk_toplevel_begin_resize(GDK_TOPLEVEL(surface), static_cast<GdkSurfaceEdge>(translated), cursor, 0, 0, 0, 1);
    }

    void window::set_minimized(bool enabled)
    {
        emit_warning<gtk_buggy_warning>();

        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_minimized(true); }).get();
        }

        if (!enabled)
        {
            gtk_window_unminimize(GTK_WINDOW(m_impl->window));
            return;
        }

        gtk_window_minimize(GTK_WINDOW(m_impl->window));
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_maximized(true); }).get();
        }

        if (!enabled)
        {
            gtk_window_unmaximize(GTK_WINDOW(m_impl->window));
            return;
        }

        gtk_window_maximize(GTK_WINDOW(m_impl->window));
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(true); }).get();
        }

        gtk_window_set_resizable(GTK_WINDOW(m_impl->window), enabled);
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(true); }).get();
        }

        gtk_window_set_decorated(GTK_WINDOW(m_impl->window), enabled);
    }

    void window::set_always_on_top(bool) // NOLINT
    {
        emit_warning<gtk_warning>();
    }

    void window::set_icon(const icon &icon)
    {
        // TODO: Implement
    }

    void window::set_title(const std::string &title)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, title] { return set_title(title); }).get();
        }

        gtk_window_set_title(GTK_WINDOW(m_impl->window), title.c_str());
    }

    void window::set_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, width, height] { return set_size(width, height); }).get();
        }

        gtk_window_set_default_size(GTK_WINDOW(m_impl->window), width, height);
    }

    void window::set_max_size(int, int) // NOLINT
    {
        emit_warning<gtk_warning>();
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return dispatch([this, width, height] { return set_min_size(width, height); }).get();
        }

        gtk_widget_set_size_request(GTK_WIDGET(m_impl->window), width, height);
    }

    template <>
    void window::run<true>()
    {
        auto callback = [](GtkApplication *, gpointer *data)
        {
            // The "real" callback is registered in the constructor already, however, due to the non-async workaround,
            // we register it here again to silence GIO warnings.
        };

        g_signal_connect(impl::application.get(), "activate", G_CALLBACK(+callback), nullptr);
        g_application_run(G_APPLICATION(impl::application.get()), 0, nullptr);
    }

    template <>
    void window::run<false>()
    {
        // https://github.com/GNOME/glib/blob/ce5e11aef4be46594941662a521c7f5e026cfce9/gio/gapplication.c#L2591

        static auto init = false;
        auto *context    = g_main_context_default();

        if (!g_main_context_acquire(context))
        {
            return;
        }

        if (!init && g_application_register(G_APPLICATION(impl::application.get()), nullptr, nullptr))
        {
            g_application_activate(G_APPLICATION(impl::application.get()));
            init = true;
        }

        g_main_context_iteration(context, false);
        g_main_context_release(context);
    }
} // namespace saucer
