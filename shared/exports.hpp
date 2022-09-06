#pragma once
#include <cstdint>
#include "serializers/ffi.hpp"

#define requires_free

namespace saucer
{
    class window;
    class webview;
    class smartview;
    class embedded_file;
} // namespace saucer

extern "C"
{
    using std::size_t;
    using std::uint64_t;

    bool window_get_resizable(const saucer::window *window);
    void window_get_title(const saucer::window *window, char *output, size_t output_size);
    bool window_get_decorations(const saucer::window *window);
    bool window_get_always_on_top(const saucer::window *window);
    void window_get_size(const saucer::window *window, size_t *width, size_t *height);
    void window_get_max_size(const saucer::window *window, size_t *width, size_t *height);
    void window_get_min_size(const saucer::window *window, size_t *width, size_t *height);
    void window_get_background_color(const saucer::window *window, size_t *r, size_t *g, size_t *b, size_t *a);

    void window_hide(saucer::window *window);
    void window_show(saucer::window *window);
    void window_close(saucer::window *window);

    void window_set_resizable(saucer::window *window, bool enabled);
    void window_set_title(saucer::window *window, const char *title);
    void window_set_decorations(saucer::window *window, bool enabled);
    void window_set_always_on_top(saucer::window *window, bool enabled);
    void window_set_size(saucer::window *window, size_t width, size_t height);
    void window_set_max_size(saucer::window *window, size_t width, size_t height);
    void window_set_min_size(saucer::window *window, size_t width, size_t height);
    void window_set_background_color(saucer::window *window, size_t r, size_t g, size_t b, size_t a);

    enum window_event
    {
        resize,
        close
    };

    void window_clear(saucer::window *window, window_event event);
    void window_remove(saucer::window *window, window_event event, uint64_t id);

    uint64_t window_on_resize(saucer::window *window, void (*callback)(size_t, size_t));
    uint64_t window_on_close(saucer::window *window, bool (*callback)());

    void window_run();
}

extern "C"
{
    bool webview_get_dev_tools(const saucer::webview *webview);
    void webview_get_url(const saucer::webview *webview, char *output, size_t output_size);
    bool webview_get_transparent(const saucer::webview *webview);
    bool webview_get_context_menu(const saucer::webview *webview);

    void webview_serve_embedded(saucer::webview *webview, const char *file);

    void webview_set_dev_tools(saucer::webview *webview, bool enabled);
    void webview_set_context_menu(saucer::webview *webview, bool enabled);
    void webview_set_url(saucer::webview *webview, const char *url);

    struct ffi_embedded_file;

    [[requires_free]] ffi_embedded_file *embedded_file_new(const char *name, const char *mime, std::size_t size, const std::uint8_t *data);
    void embedded_file_free(ffi_embedded_file *file);

    void webview_embed_files(saucer::webview *webview, ffi_embedded_file **files, std::size_t file_count);

    void webview_set_transparent(saucer::webview *webview, bool enabled, bool blur = false);

    void webview_clear_scripts(saucer::webview *webview);

    void webview_clear_embedded(saucer::webview *webview);

    void webview_run_java_script(saucer::webview *webview, const char *java_script);

    enum load_time
    {
        ready,
        creation,
    };

    void webview_inject(saucer::webview *webview, const char *java_script, load_time load_time);

    enum web_event
    {
        url_changed
    };

    void webview_clear(saucer::webview *webview, web_event event);
    void webview_remove(saucer::webview *webview, web_event event, uint64_t id);

    uint64_t webview_on_url_changed(saucer::webview *webview, void (*callback)(const char *));
}

extern "C"
{
    saucer::ffi_function_data *ffi_function_data_new(std::size_t id, const char *function, void *data);

    void ffi_function_data_get_function(saucer::ffi_function_data *data, char *output, std::size_t output_size);
    void *ffi_function_data_get_data(saucer::ffi_function_data *data);
    std::size_t ffi_function_data_get_id(saucer::ffi_function_data *data);
    saucer::ffi_result_data *ffi_result_data_new(std::size_t id, void *data);

    void *ffi_result_data_get_data(saucer::ffi_result_data *data);
    std::size_t ffi_result_data_get_id(saucer::ffi_result_data *data);

    void ffi_serializer_set_buffer_size(std::size_t size);
    void ffi_serializer_set_init_script(const char *script);
    void ffi_serializer_set_js_serializer(const char *serializer);
    void ffi_serializer_set_parse_callback(saucer::parse_callback_t);
    void ffi_serializer_set_serialize_callback(saucer::serialize_callback_t);
}

extern "C"
{
    [[requires_free]] saucer::smartview *smartview_new();
    void smartview_free(saucer::smartview *smartview);

    void smartview_expose(saucer::smartview *smartview, const char *name, bool async);

    // TODO: evals / promises
    // TODO: modules
}