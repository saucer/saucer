#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "window.h"
#include "scheme.h"

#include "options.h"

#include <stdbool.h>

    enum SAUCER_LOAD_TIME
    {
        SAUCER_LOAD_TIME_CREATION,
        SAUCER_LOAD_TIME_READY,
    };

    enum SAUCER_WEB_EVENT
    {
        SAUCER_WEB_EVENT_TITLE_CHANGED,
        SAUCER_WEB_EVENT_LOAD_FINISHED,
        SAUCER_WEB_EVENT_ICON_CHANGED,
        SAUCER_WEB_EVENT_LOAD_STARTED,
        SAUCER_WEB_EVENT_URL_CHANGED,
        SAUCER_WEB_EVENT_DOM_READY,
    };

    struct saucer_embedded_file;

    /*[[sc::requires_free]]*/ saucer_embedded_file *saucer_embed(saucer_stash *content, const char *mime);
    void saucer_embed_free(saucer_embedded_file *);

    /*[[sc::requires_free]]*/ saucer_handle *saucer_new(saucer_options *options);
    void saucer_free(saucer_handle *);

    typedef bool (*saucer_on_message)(const char *);
    void saucer_webview_on_message(saucer_handle *, saucer_on_message);

    /*[[sc::requires_free]]*/ saucer_icon *saucer_webview_favicon(saucer_handle *);
    void saucer_webview_background(saucer_handle *, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
    /*[[sc::requires_free]]*/ char *saucer_webview_page_title(saucer_handle *);

    bool saucer_webview_dev_tools(saucer_handle *);
    /*[[sc::requires_free]]*/ char *saucer_webview_url(saucer_handle *);
    bool saucer_webview_context_menu(saucer_handle *);

    void saucer_webview_set_dev_tools(saucer_handle *, bool enabled);
    void saucer_webview_set_context_menu(saucer_handle *, bool enabled);
    void saucer_webview_set_background(saucer_handle *, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    void saucer_webview_set_file(saucer_handle *, const char *file);
    void saucer_webview_set_url(saucer_handle *, const char *url);

    void saucer_webview_embed_file(saucer_handle *, const char *name, saucer_embedded_file *file);
    void saucer_webview_serve(saucer_handle *, const char *file);
    void saucer_webview_serve_scheme(saucer_handle *, const char *file, const char *scheme);

    void saucer_webview_clear_scripts(saucer_handle *);

    void saucer_webview_clear_embedded(saucer_handle *);
    void saucer_webview_clear_embedded_file(saucer_handle *, const char *file);

    void saucer_webview_execute(saucer_handle *, const char *java_script);
    void saucer_webview_inject(saucer_handle *, const char *java_script, SAUCER_LOAD_TIME load_time);

    void saucer_webview_handle_scheme(saucer_handle *, const char *name, saucer_scheme_handler handler);
    void saucer_webview_remove_scheme(saucer_handle *, const char *name);

    void saucer_webview_clear(saucer_handle *, SAUCER_WEB_EVENT event);
    void saucer_webview_remove(saucer_handle *, SAUCER_WEB_EVENT event, uint64_t id);

    void saucer_webview_once(saucer_handle *, SAUCER_WEB_EVENT event, void *callback);
    uint64_t saucer_webview_on(saucer_handle *, SAUCER_WEB_EVENT event, void *callback);

    void saucer_register_scheme(const char *name);

#ifdef __cplusplus
}
#endif
