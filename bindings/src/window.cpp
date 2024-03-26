#include "window.h"

#include "utils.hpp"
#include "common.impl.hpp"

#include <cstring>
#include <saucer/smartview.hpp>

extern "C"
{
    bool saucer_window_focused(saucer_handle *handle)
    {
        return handle->focused();
    }

    bool saucer_window_minimized(saucer_handle *handle)
    {
        return handle->minimized();
    }

    bool saucer_window_maximized(saucer_handle *handle)
    {
        return handle->maximized();
    }

    bool saucer_window_resizable(saucer_handle *handle)
    {
        return handle->resizable();
    }

    bool saucer_window_decorations(saucer_handle *handle)
    {
        return handle->decorations();
    }

    bool saucer_window_always_on_top(saucer_handle *handle)
    {
        return handle->always_on_top();
    }

    saucer_color *saucer_window_background(saucer_handle *handle)
    {
        return cast(new saucer::color{handle->background()});
    }

    char *saucer_window_title(saucer_handle *handle)
    {
        auto title = handle->title();
        auto *rtn  = reinterpret_cast<char *>(saucer_alloc(title.size() + 1));

        strcpy(rtn, title.c_str());

        return rtn;
    }

    void saucer_window_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->size();
    }

    void saucer_window_max_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->max_size();
    }

    void saucer_window_min_size(saucer_handle *handle, int *width, int *height)
    {
        std::tie(*width, *height) = handle->min_size();
    }

    void saucer_window_hide(saucer_handle *handle)
    {
        handle->hide();
    }

    void saucer_window_show(saucer_handle *handle)
    {
        handle->show();
    }

    void saucer_window_close(saucer_handle *handle)
    {
        handle->close();
    }

    void saucer_window_focus(saucer_handle *handle)
    {
        handle->focus();
    }

    void saucer_window_start_drag(saucer_handle *handle)
    {
        handle->start_drag();
    }

    void saucer_window_start_resize(saucer_handle *handle, saucer_window_edge edge)
    {
        handle->start_resize(static_cast<saucer::window_edge>(edge));
    }

    void saucer_window_set_minimized(saucer_handle *handle, bool enabled)
    {
        handle->set_minimized(enabled);
    }

    void saucer_window_set_maximized(saucer_handle *handle, bool enabled)
    {
        handle->set_maximized(enabled);
    }

    void saucer_window_set_resizable(saucer_handle *handle, bool enabled)
    {
        handle->set_resizable(enabled);
    }

    void saucer_window_set_decorations(saucer_handle *handle, bool enabled)
    {
        handle->set_decorations(enabled);
    }

    void saucer_window_set_always_on_top(saucer_handle *handle, bool enabled)
    {
        handle->set_always_on_top(enabled);
    }

    void saucer_window_set_title(saucer_handle *handle, const char *title)
    {
        handle->set_title(title);
    }

    void saucer_window_set_background(saucer_handle *handle, saucer_color *color)
    {
        handle->set_background(*cast(color));
    }

    void saucer_window_set_size(saucer_handle *handle, int width, int height)
    {
        handle->set_size(width, height);
    }

    void saucer_window_set_max_size(saucer_handle *handle, int width, int height)
    {
        handle->set_max_size(width, height);
    }

    void saucer_window_set_min_size(saucer_handle *handle, int width, int height)
    {
        handle->set_min_size(width, height);
    }

    void saucer_window_clear(saucer_handle *handle, saucer_window_event event)
    {
        handle->clear(static_cast<saucer::window_event>(event));
    }

    void saucer_window_remove(saucer_handle *handle, saucer_window_event event, uint64_t id)
    {
        handle->remove(static_cast<saucer::window_event>(event), id);
    }

    void saucer_window_once(saucer_handle *handle, saucer_window_event event, void *callback)
    {
        auto cb = cast_callback{callback, handle};

        switch (event)
        {
        case SAUCER_WINDOW_EVENT_MAXIMIZE:
            return handle->once<saucer::window_event::maximize>(cb);
        case SAUCER_WINDOW_EVENT_MINIMIZE:
            return handle->once<saucer::window_event::minimize>(cb);
        case SAUCER_WINDOW_EVENT_CLOSED:
            return handle->once<saucer::window_event::closed>(cb);
        case SAUCER_WINDOW_EVENT_RESIZE:
            return handle->once<saucer::window_event::resize>(cb);
        case SAUCER_WINDOW_EVENT_FOCUS:
            return handle->once<saucer::window_event::focus>(cb);
        case SAUCER_WINDOW_EVENT_CLOSE:
            return handle->once<saucer::window_event::close>(cb);
        }
    }

    uint64_t saucer_window_on(saucer_handle *handle, saucer_window_event event, void *callback)
    {
        auto cb = cast_callback{callback, handle};

        switch (event)
        {
        case SAUCER_WINDOW_EVENT_MAXIMIZE:
            return handle->on<saucer::window_event::maximize>(cb);
        case SAUCER_WINDOW_EVENT_MINIMIZE:
            return handle->on<saucer::window_event::minimize>(cb);
        case SAUCER_WINDOW_EVENT_CLOSED:
            return handle->on<saucer::window_event::closed>(cb);
        case SAUCER_WINDOW_EVENT_RESIZE:
            return handle->on<saucer::window_event::resize>(cb);
        case SAUCER_WINDOW_EVENT_FOCUS:
            return handle->on<saucer::window_event::focus>(cb);
        case SAUCER_WINDOW_EVENT_CLOSE:
            return handle->on<saucer::window_event::close>(cb);
        }
    }

    void saucer_window_run(saucer_handle *handle)
    {
        handle->run();
    }

    void saucer_window_run_once(saucer_handle *handle)
    {
        handle->run<false>();
    }
}
