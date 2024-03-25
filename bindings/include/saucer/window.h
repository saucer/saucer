#pragma once

#include "color.h"
#include "common.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    enum saucer_window_edge
    {
        SAUCER_WINDOW_EDGE_TOP    = 1 << 0,
        SAUCER_WINDOW_EDGE_BOTTOM = 1 << 1,
        SAUCER_WINDOW_EDGE_LEFT   = 1 << 2,
        SAUCER_WINDOW_EDGE_RIGHT  = 1 << 3,
    };

    enum saucer_window_event
    {
        SAUCER_WINDOW_EVENT_MAXIMIZE,
        SAUCER_WINDOW_EVENT_MINIMIZE,
        SAUCER_WINDOW_EVENT_CLOSED,
        SAUCER_WINDOW_EVENT_RESIZE,
        SAUCER_WINDOW_EVENT_FOCUS,
        SAUCER_WINDOW_EVENT_CLOSE,
    };

    bool saucer_window_focused(saucer_handle *);
    bool saucer_window_minimized(saucer_handle *);
    bool saucer_window_maximized(saucer_handle *);

    bool saucer_window_resizable(saucer_handle *);
    bool saucer_window_decorations(saucer_handle *);
    bool saucer_window_always_on_top(saucer_handle *);

    /*[[requires_free]]*/ saucer_color *saucer_window_background(saucer_handle *);
    /*[[requires_free]]*/ char *saucer_window_title(saucer_handle *);

    void saucer_window_size(saucer_handle *, int *width, int *height);
    void saucer_window_max_size(saucer_handle *, int *width, int *height);
    void saucer_window_min_size(saucer_handle *, int *width, int *height);

    void saucer_window_hide(saucer_handle *);
    void saucer_window_show(saucer_handle *);
    void saucer_window_close(saucer_handle *);

    void saucer_window_focus(saucer_handle *);

    void saucer_window_start_drag(saucer_handle *);
    void saucer_window_start_resize(saucer_handle *, saucer_window_edge edge);

    void saucer_window_set_minimized(saucer_handle *, bool enabled);
    void saucer_window_set_maximized(saucer_handle *, bool enabled);

    void saucer_window_set_resizable(saucer_handle *, bool enabled);
    void saucer_window_set_decorations(saucer_handle *, bool enabled);
    void saucer_window_set_always_on_top(saucer_handle *, bool enabled);

    void saucer_window_set_title(saucer_handle *, const char *title);
    void saucer_window_set_background(saucer_handle *, saucer_color *color);

    void saucer_window_set_size(saucer_handle *, int width, int height);
    void saucer_window_set_max_size(saucer_handle *, int width, int height);
    void saucer_window_set_min_size(saucer_handle *, int width, int height);

    void saucer_window_clear(saucer_handle *, saucer_window_event event);
    void saucer_window_remove(saucer_handle *, saucer_window_event event, uint64_t id);

    /*
     * `callback` should be a function pointer to a function matching the event,
     * that is: <return-type>(saucer_handle *, <params>...);
     *
     * where "<return-type>" and "<params>..." are to be
     * substituted according to the given event signature (see the respective c++ header)
     */

    void saucer_window_once(saucer_handle *, saucer_window_event event, void *callback);
    uint64_t saucer_window_on(saucer_handle *, saucer_window_event event, void *callback);

    void saucer_window_run(saucer_handle *);
    void saucer_window_run_once(saucer_handle *);

#ifdef __cplusplus
}
#endif
