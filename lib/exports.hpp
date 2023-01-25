#pragma once
#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

struct embedded_file;

namespace saucer
{
    class smartview;
    struct result_data;
    struct message_data;
    struct function_data;
    struct webview_options;
} // namespace saucer

#else
#include <stddef.h>
#include <stdint.h>
#define bool int

#define declare(name)                                                                                                  \
    struct _##name;                                                                                                    \
    typedef struct _##name name;

declare(embedded_file);

declare(smartview);
declare(result_data);
declare(message_data);
declare(function_data);
declare(webview_options);
#endif

#ifdef __cplusplus
extern "C"
{
    using saucer::function_data;
    using saucer::message_data;
    using saucer::result_data;
    using saucer::smartview;
    using saucer::webview_options;
    using std::size_t;
    using std::uint8_t;
#endif
    smartview *smartview_new(webview_options *);
    void smartview_free(smartview *);

    // ? Options
    webview_options *webview_options_new();
    void webview_options_free(webview_options *);

    void webview_options_set_persistent_cookies(webview_options *, bool);
    void webview_options_set_storage_path(webview_options *, const char *);

    //? Window

    bool smartview_get_resizable(smartview *);
    void smartview_get_title(smartview *, char *, size_t);
    bool smartview_get_decorations(smartview *);
    bool smartview_get_always_on_top(smartview *);

    void smartview_get_size(smartview *, int *, int *);
    void smartview_get_max_size(smartview *, int *, int *);
    void smartview_get_min_size(smartview *, int *, int *);
    void smartview_get_background_color(smartview *, int *, int *, int *, int *);

    void smartview_hide(smartview *);
    void smartview_show(smartview *);
    void smartview_close(smartview *);

    void smartview_set_resizable(smartview *, bool);
    void smartview_set_title(smartview *, const char *);
    void smartview_set_decorations(smartview *, bool);
    void smartview_set_always_on_top(smartview *, bool);

    void smartview_set_size(smartview *, int, int);
    void smartview_set_max_size(smartview *, int, int);
    void smartview_set_min_size(smartview *, int, int);
    void smartview_set_background_color(smartview *, int, int, int, int);

    //? Webview

    enum load_time
    {
        LOAD_TIME_CREATION,
        LOAD_TIME_READY,
    };

#ifndef __cplusplus
#define load_time enum load_time
#endif

    bool smartview_get_dev_tools(smartview *);
    void smartview_get_url(smartview *, char *, size_t);
    bool smartview_get_transparent(smartview *);
    bool smartview_get_context_menu(smartview *);

    void smartview_clear_scripts(smartview *);
    void smartview_clear_embedded(smartview *);
    void smartview_run_java_script(smartview *, const char *);
    void smartview_inject(smartview *, const char *, load_time);

    void smartview_set_dev_tools(smartview *, bool);
    void smartview_set_context_menu(smartview *, bool);
    void smartview_set_url(smartview *, const char *);
    void smartview_set_transparent(smartview *, bool, bool);

#ifndef __cplusplus
#undef load_time
#endif

    // ? Embedding

    embedded_file *embedded_file_new(const char *, const char *, size_t, const uint8_t *);
    void embedded_file_free(embedded_file *);

    void smartview_serve(smartview *, const char *);
    void smartview_embed(smartview *, embedded_file **, size_t);

    // ? Events

    enum event
    {
        EVENT_RESIZE,
        EVENT_CLOSE,

        EVENT_URL_CHANGED,
    };

#ifndef __cplusplus
#define event enum event
#endif

    void smartview_clear(smartview *, event);
    void smartview_remove(smartview *, event, uint64_t);

#ifndef __cplusplus
#undef event
#endif

    uint64_t smartview_on_close(smartview *, bool (*)());
    uint64_t smartview_on_resize(smartview *, void (*)(size_t, size_t));
    uint64_t smartview_on_url_changed(smartview *, void (*)(const char *));

    // ? Serializers

    void serializer_set_buffer_size(size_t);
    void serializer_set_init_script(const char *);
    void serializer_set_js_serializer(const char *);

    void serializer_set_parse_callback(message_data *(*)(const char *));
    void serializer_set_serializer_callback(bool (*)(smartview *, function_data *, char *, size_t, size_t *, int *));

    function_data *function_data_new(size_t, const char *, void *);
    void function_data_get_function(function_data *, char *, size_t);
    void *function_data_get_data(function_data *);
    size_t function_data_get_id(function_data *);
    void function_data_free(function_data *);

    result_data *result_data_new(size_t, void *);
    void *result_data_get_data(result_data *);
    size_t result_data_get_id(result_data *);
    void result_data_free(result_data *);

    // ? Smartview

    void smartview_expose(smartview *, const char *, bool);

#ifdef __cplusplus
}
#endif