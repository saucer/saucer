#pragma once
#include <cstddef>

#if defined(_WIN32)
#define __export __declspec(dllexport)
#else
#define __export __attribute__((visibility("default")))
#endif

namespace saucer
{
    class window;
} // namespace saucer

extern "C"
{
    extern __export bool window_get_resizeable(saucer::window *);
    extern __export bool window_get_decorations(saucer::window *);
    extern __export bool window_get_always_on_top(saucer::window *);
    extern __export void window_get_title(saucer::window *, char *, size_t);
    extern __export void window_get_size(saucer::window *, size_t *, size_t *);
    extern __export void window_get_max_size(saucer::window *, size_t *, size_t *);
    extern __export void window_get_min_size(saucer::window *, size_t *, size_t *);

    extern __export void window_hide(saucer::window *);
    extern __export void window_show(saucer::window *);
    extern __export void window_exit(saucer::window *);

    extern __export void window_run(saucer::window *);

    extern __export void window_on_close(saucer::window *, bool (*)());
    extern __export void window_on_resize(saucer::window *, void (*)(size_t, size_t));

    extern __export void window_set_resizeable(saucer::window *, bool);
    extern __export void window_set_decorations(saucer::window *, bool);
    extern __export void window_set_always_on_top(saucer::window *, bool);
    extern __export void window_set_title(saucer::window *, const char *);
    extern __export void window_set_size(saucer::window *, size_t, size_t);
    extern __export void window_set_max_size(saucer::window *, size_t, size_t);
    extern __export void window_set_min_size(saucer::window *, size_t, size_t);
}

namespace saucer
{
    class webview;
} // namespace saucer

extern "C"
{
    struct embedded_file
    {
        const char *file_name;
        const char *mime_type;
        size_t size;
        const unsigned char *data;
    };

    enum load_time
    {
        LOAD_TIME_READY,
        LOAD_TIME_CREATION
    };

    extern __export bool webview_get_dev_tools(saucer::webview *);
    extern __export bool webview_get_context_menu(saucer::webview *);
    extern __export void webview_get_url(saucer::webview *, char *, size_t);

    extern __export void webview_set_dev_tools(saucer::webview *, bool);
    extern __export void webview_set_url(saucer::webview *, const char *);
    extern __export void webview_set_context_menu(saucer::webview *, bool);
    extern __export void webview_serve_embedded(saucer::webview *, const char *);
    extern __export void webview_embed_files(saucer::webview *, embedded_file **, size_t);

    extern __export void webview_run_java_script(saucer::webview *, const char *);
    extern __export void webview_inject(saucer::webview *, const char *, load_time);

    extern __export void webview_clear_scripts(saucer::webview *);
    extern __export void webview_clear_embedded(saucer::webview *);

    extern __export void webview_on_url_changed(saucer::webview *, void (*)(const char *));
}

namespace saucer
{
    struct result_data;
    struct message_data;
} // namespace saucer

extern "C"
{
    enum serializer_error
    {
        SERIALIZER_ERROR_ARGUMENT_MISMATCH,
        SERIALIZER_ERROR_PARSER_MISMATCH,
        SERIALIZER_ERROR_NONE,
    };

    struct ffi_promise;
    struct ffi_smartview;
    struct ffi_serializer;
    struct ffi_result_data;
    struct ffi_function_data;

    using on_message_callback_t = void (*)(const char *);
    using ffi_resolve_callback_t = bool (*)(ffi_result_data *);
    using ffi_parse_callback_t = saucer::message_data *(*)(const char *);
    using ffi_callback_t = bool (*)(ffi_function_data *, void (**)(char *, size_t));

    extern __export ffi_smartview *smartview_new();
    extern __export void smartview_free(ffi_smartview *);

    extern __export void smartview_add_callback(ffi_smartview *, const char *, ffi_callback_t, bool);
    extern __export void smartview_add_eval(ffi_smartview *, ffi_promise *, ffi_resolve_callback_t, const char *);

    extern __export void smartview_reject(ffi_smartview *, size_t, const char *);
    extern __export void smartview_resolve(ffi_smartview *, size_t, const char *);
    extern __export void smartview_set_on_message_callback(ffi_smartview *, on_message_callback_t);

    extern __export void smartview_serializer_set_initialization_script(ffi_smartview *, const char *);
    extern __export void smartview_serializer_set_parse_callback(ffi_smartview *, ffi_parse_callback_t);
    extern __export void smartview_serializer_set_java_script_deserializer(ffi_smartview *, const char *);

    extern __export ffi_result_data *ffi_result_data_new();
    extern __export size_t ffi_result_data_get_id(ffi_result_data *);
    extern __export void *ffi_result_data_get_data(ffi_result_data *);
    extern __export void ffi_result_data_set_id(ffi_result_data *, size_t);
    extern __export void ffi_result_data_set_data(ffi_result_data *, void *);

    extern __export ffi_function_data *ffi_function_data_new();
    extern __export size_t ffi_function_data_get_id(ffi_function_data *);
    extern __export void *ffi_function_data_get_data(ffi_function_data *);
    extern __export void ffi_function_data_set_id(ffi_function_data *, size_t);
    extern __export void ffi_function_data_set_data(ffi_function_data *, void *);
    extern __export void ffi_function_data_set_function(ffi_function_data *, const char *);
    extern __export void ffi_function_data_get_function(ffi_function_data *, char *, size_t);
    //? ffi_result/function_data_new_free is not needed because the lifetime management is done by the smartview

    extern __export ffi_promise *ffi_promise_new();
    extern __export void ffi_promise_set_fail_callback(ffi_promise *, void (*)());
    //? ffi_promise_free is not needed because the lifetime management is done by the smartview
}