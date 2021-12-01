#pragma once
#include <cstddef>

#if defined(_WIN32)
#define __export __declspec(dllexport)
#else
#define __export __attribute__((visibility("default")))
#endif

struct embedded_file;
namespace saucer
{
    class window;
    class webview;
    class bridged_webview;
} // namespace saucer

extern "C"
{
    extern void __export window_run();
    extern void __export window_show(saucer::window *);
    extern void __export window_hide(saucer::window *);

    extern void __export window_set_title(saucer::window *, const char *);
    extern void __export window_get_title(saucer::window *, char *, unsigned int);

    extern void __export window_set_resizeable(saucer::window *, bool);
    extern void __export window_set_decorations(saucer::window *, bool);
    extern void __export window_set_always_on_top(saucer::window *, bool);

    extern bool __export window_get_resizeable(saucer::window *);
    extern bool __export window_get_decorations(saucer::window *);
    extern bool __export window_get_always_on_top(saucer::window *);

    extern void __export window_set_size(saucer::window *, size_t, size_t);
    extern void __export window_set_max_size(saucer::window *, size_t, size_t);
    extern void __export window_set_min_size(saucer::window *, size_t, size_t);

    extern void __export window_get_size(saucer::window *, size_t *, size_t *);
    extern void __export window_get_max_size(saucer::window *, size_t *, size_t *);
    extern void __export window_get_min_size(saucer::window *, size_t *, size_t *);

    extern void __export window_on_close(saucer::window *, bool (*)());
    extern void __export window_on_resize(saucer::window *, void (*)(size_t, size_t));
}

extern "C"
{
    extern void saucer_free_embedded_file(embedded_file *);
    extern embedded_file *saucer_make_embedded_file(const char *, const char *, size_t, const unsigned char *);

    extern __export saucer::webview *webview_new();
    extern __export saucer::bridged_webview *bridged_webview_new();

    extern __export void webview_free(saucer::webview *);
    extern __export void bridged_webview_free(saucer::bridged_webview *);

    extern bool __export webview_get_dev_tools(saucer::webview *);
    extern bool __export webview_get_context_menu(saucer::webview *);
    extern void __export webview_get_url(saucer::webview *, char *, unsigned int);

    extern void __export webview_set_dev_tools(saucer::webview *, bool);
    extern void __export webview_set_url(saucer::webview *, const char *);
    extern void __export webview_set_context_menu(saucer::webview *, bool);
    extern void __export webview_serve_embedded(saucer::webview *, const char *);

    extern void __export webview_inject(saucer::webview *, const char *, char);
    extern void __export webview_run_java_script(saucer::webview *, const char *);
    extern void __export webview_embed_files(saucer::webview *, embedded_file **, size_t);

    extern void __export webview_clear_scripts(saucer::webview *);
    extern void __export webview_clear_embedded(saucer::webview *);

    extern void __export webview_on_url_changed(saucer::webview *, void (*)(const char *));
    extern void __export bridged_webview_on_message(saucer::bridged_webview *, void (*)(const char *));
}