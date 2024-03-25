#pragma once

#include "embed.h"
#include "common.h"

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    enum saucer_load_time
    {
        SAUCER_LOAD_TIME_CREATION,
        SAUCER_LOAD_TIME_READY,
    };

    enum saucer_web_event
    {
        SAUCER_WEB_EVENT_LOAD_FINISHED,
        SAUCER_WEB_EVENT_LOAD_STARTED,
        SAUCER_WEB_EVENT_URL_CHANGED,
        SAUCER_WEB_EVENT_DOM_READY,
    };

    bool saucer_webview_dev_tools(saucer_handle *);
    /*[[requires_free]]*/ char *saucer_webview_url(saucer_handle *);
    bool saucer_webview_context_menu(saucer_handle *);

    void saucer_webview_set_dev_tools(saucer_handle *, bool enabled);
    void saucer_webview_set_context_menu(saucer_handle *, bool enabled);
    void saucer_webview_set_url(saucer_handle *, const char *url);

    void saucer_webview_embed(saucer_handle *, saucer_embedded_files *files);
    void saucer_webview_serve(saucer_handle *, const char *file);

    void saucer_webview_clear_scripts(saucer_handle *);
    void saucer_webview_clear_embedded(saucer_handle *);

    void saucer_webview_execute(saucer_handle *, const char *java_script);
    void saucer_webview_inject(saucer_handle *, const char *java_script, saucer_load_time load_time);

    void saucer_webview_clear(saucer_handle *, saucer_web_event event);
    void saucer_webview_remove(saucer_handle *, saucer_web_event event, uint64_t id);

    /*
     * see "window.h" on callbacks
     */

    void saucer_webview_once(saucer_handle *, saucer_web_event event, void *callback);
    uint64_t saucer_webview_on(saucer_handle *, saucer_web_event event, void *callback);

#ifdef __cplusplus
}
#endif
