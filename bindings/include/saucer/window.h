#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "icon.h"

#include <stdint.h>
#include <stdbool.h>

    enum SAUCER_WINDOW_EVENT
    {
        SAUCER_WINDOW_EVENT_MAXIMIZE,
        SAUCER_WINDOW_EVENT_MINIMIZE,
        SAUCER_WINDOW_EVENT_CLOSED,
        SAUCER_WINDOW_EVENT_RESIZE,
        SAUCER_WINDOW_EVENT_FOCUS,
        SAUCER_WINDOW_EVENT_CLOSE,
    };

    enum SAUCER_WINDOW_EDGE
    {
        SAUCER_WINDOW_EDGE_TOP,
        SAUCER_WINDOW_EDGE_BOTTOM,
        SAUCER_WINDOW_EDGE_LEFT,
        SAUCER_WINDOW_EDGE_RIGHT,
    };

    struct saucer_handle;

    bool saucer_window_focused(saucer_handle *);
    bool saucer_window_minimized(saucer_handle *);
    bool saucer_window_maximized(saucer_handle *);

    bool saucer_window_resizable(saucer_handle *);
    bool saucer_window_decorations(saucer_handle *);
    bool saucer_window_always_on_top(saucer_handle *);

    /*[[sc::requires_free]]*/ char *saucer_window_title(saucer_handle *);

    void saucer_window_size(saucer_handle *, int *width, int *height);
    void saucer_window_max_size(saucer_handle *, int *width, int *height);
    void saucer_window_min_size(saucer_handle *, int *width, int *height);

    void saucer_window_hide(saucer_handle *);
    void saucer_window_show(saucer_handle *);
    void saucer_window_close(saucer_handle *);

    void saucer_window_focus(saucer_handle *);

    void saucer_window_start_drag(saucer_handle *);
    void saucer_window_start_resize(saucer_handle *, SAUCER_WINDOW_EDGE edge);

    void saucer_window_set_minimized(saucer_handle *, bool enabled);
    void saucer_window_set_maximized(saucer_handle *, bool enabled);

    void saucer_window_set_resizable(saucer_handle *, bool enabled);
    void saucer_window_set_decorations(saucer_handle *, bool enabled);
    void saucer_window_set_always_on_top(saucer_handle *, bool enabled);

    void saucer_window_set_icon(saucer_handle *, saucer_icon *icon);
    void saucer_window_set_title(saucer_handle *, const char *title);

    void saucer_window_set_size(saucer_handle *, int width, int height);
    void saucer_window_set_max_size(saucer_handle *, int width, int height);
    void saucer_window_set_min_size(saucer_handle *, int width, int height);

    void saucer_window_clear(saucer_handle *, SAUCER_WINDOW_EVENT event);
    void saucer_window_remove(saucer_handle *, SAUCER_WINDOW_EVENT event, uint64_t id);

    /**
     * @note See respective comment on events in "window.h"
     * @warning The icon passed to the @param callback in the `web_event::icon_changed` event must be free'd by the
     * user
     */

    void saucer_window_once(saucer_handle *, SAUCER_WINDOW_EVENT event, void *callback);
    uint64_t saucer_window_on(saucer_handle *, SAUCER_WINDOW_EVENT event, void *callback);

    typedef void (*saucer_window_callback)();
    void saucer_window_dispatch(saucer_handle *, saucer_window_callback callback);

    void saucer_window_run();
    void saucer_window_run_once();

#ifdef __cplusplus
}
#endif
