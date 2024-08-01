#include "window.h"
#include "memory.h"

#include "icon.hpp"
#include "webview.hpp"

#include <cstring>
#include <saucer/window.hpp>

extern "C"
{
    bool saucer_window_focused(saucer_handle *handle)
    {
        return handle->value().focused();
    }

    bool saucer_window_minimized(saucer_handle *handle)
    {
        return handle->value().minimized();
    }

    bool saucer_window_maximized(saucer_handle *handle)
    {
        return handle->value().maximized();
    }

    bool saucer_window_resizable(saucer_handle *handle)
    {
        return handle->value().resizable();
    }

    bool saucer_window_decorations(saucer_handle *handle)
    {
        return handle->value().decorations();
    }

    bool saucer_window_always_on_top(saucer_handle *handle)
    {
        return handle->value().always_on_top();
    }

    char *saucer_window_title(saucer_handle *handle)
    {
        auto title = handle->value().title();

        auto *rtn = static_cast<char *>(saucer_alloc(title.capacity()));
        strncpy(rtn, title.data(), title.capacity());

        return rtn;
    }

    void saucer_window_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->value().size();
    }

    void saucer_window_max_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->value().max_size();
    }

    void saucer_window_min_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->value().min_size();
    }

    void saucer_window_hide(saucer_handle *handle)
    {
        handle->value().hide();
    }

    void saucer_window_show(saucer_handle *handle)
    {
        handle->value().show();
    }

    void saucer_window_close(saucer_handle *handle)
    {
        handle->value().close();
    }

    void saucer_window_focus(saucer_handle *handle)
    {
        handle->value().focus();
    }

    void saucer_window_start_drag(saucer_handle *handle)
    {
        handle->value().start_drag();
    }

    void saucer_window_start_resize(saucer_handle *handle, SAUCER_WINDOW_EDGE edge)
    {
        handle->value().start_resize(static_cast<saucer::window_edge>(edge));
    }

    void saucer_window_set_minimized(saucer_handle *handle, bool enabled)
    {
        handle->value().set_minimized(enabled);
    }

    void saucer_window_set_maximized(saucer_handle *handle, bool enabled)
    {
        handle->value().set_minimized(enabled);
    }

    void saucer_window_set_resizable(saucer_handle *handle, bool enabled)
    {
        handle->value().set_resizable(enabled);
    }

    void saucer_window_set_decorations(saucer_handle *handle, bool enabled)
    {
        handle->value().set_decorations(enabled);
    }

    void saucer_window_set_always_on_top(saucer_handle *handle, bool enabled)
    {
        handle->value().set_always_on_top(enabled);
    }

    void saucer_window_set_icon(saucer_handle *handle, saucer_icon *icon)
    {
        handle->value().set_icon(icon->value());
    }

    void saucer_window_set_title(saucer_handle *handle, const char *title)
    {
        handle->value().set_title(title);
    }

    void saucer_window_set_size(saucer_handle *handle, int width, int height)
    {
        handle->value().set_size(width, height);
    }

    void saucer_window_set_max_size(saucer_handle *handle, int width, int height)
    {
        handle->value().set_max_size(width, height);
    }

    void saucer_window_set_min_size(saucer_handle *handle, int width, int height)
    {
        handle->value().set_min_size(width, height);
    }

    void saucer_window_clear(saucer_handle *handle, SAUCER_WINDOW_EVENT event)
    {
        handle->value().clear(static_cast<saucer::window_event>(event));
    }

    void saucer_window_remove(saucer_handle *handle, SAUCER_WINDOW_EVENT event, uint64_t id)
    {
        handle->value().remove(static_cast<saucer::window_event>(event), id);
    }

    void saucer_window_once(saucer_handle *handle, SAUCER_WINDOW_EVENT event, void *callback)
    {
        switch (event)
        {
        case SAUCER_WINDOW_EVENT_CLOSE:
            handle->value().once<saucer::window_event::close>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::close>>(handle, callback));
            break;
        case SAUCER_WINDOW_EVENT_CLOSED:
            handle->value().once<saucer::window_event::closed>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::closed>>(handle, callback));
            break;
        case SAUCER_WINDOW_EVENT_FOCUS:
            handle->value().once<saucer::window_event::focus>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::focus>>(handle, callback));
            break;
        case SAUCER_WINDOW_EVENT_MINIMIZE:
            handle->value().once<saucer::window_event::minimize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::minimize>>(handle, callback));
            break;
        case SAUCER_WINDOW_EVENT_MAXIMIZE:
            handle->value().once<saucer::window_event::maximize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::maximize>>(handle, callback));
            break;
        case SAUCER_WINDOW_EVENT_RESIZE:
            handle->value().once<saucer::window_event::resize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::resize>>(handle, callback));
            break;
        }
    }

    uint64_t saucer_window_on(saucer_handle *handle, SAUCER_WINDOW_EVENT event, void *callback)
    {
        switch (event)
        {
        case SAUCER_WINDOW_EVENT_CLOSE:
            return handle->value().on<saucer::window_event::close>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::close>>(handle, callback));
        case SAUCER_WINDOW_EVENT_CLOSED:
            return handle->value().on<saucer::window_event::closed>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::closed>>(handle, callback));
        case SAUCER_WINDOW_EVENT_FOCUS:
            return handle->value().on<saucer::window_event::focus>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::focus>>(handle, callback));
        case SAUCER_WINDOW_EVENT_MINIMIZE:
            return handle->value().on<saucer::window_event::minimize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::minimize>>(handle, callback));
        case SAUCER_WINDOW_EVENT_MAXIMIZE:
            return handle->value().on<saucer::window_event::maximize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::maximize>>(handle, callback));
        case SAUCER_WINDOW_EVENT_RESIZE:
            return handle->value().on<saucer::window_event::resize>(
                saucer::wrap_callback<saucer::window::events::type<saucer::window_event::resize>>(handle, callback));
        }
    }

    void saucer_window_dispatch(saucer_handle *handle, saucer_window_callback callback)
    {
        handle->value().dispatch(callback);
    }

    void saucer_window_run(saucer_handle *handle)
    {
        handle->value().run();
    }

    void saucer_window_run_once(saucer_handle *handle)
    {
        handle->value().run<false>();
    }
}
