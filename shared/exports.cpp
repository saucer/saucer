#include "serializers/ffi.hpp"

#include <utility>
#include <saucer/webview.hpp>
#include <saucer/smartview.hpp>

#if defined(_WIN32)
#define extern __export __declspec(dllexport)
#else
#define __export extern __attribute__((visibility("default")))
#endif

#define requires_free

extern "C"
{
    __export bool window_get_resizeable(const saucer::window *window)
    {
        return window->get_resizeable();
    }

    __export void window_get_title(const saucer::window *window, char *output, size_t output_size)
    {
        strcpy(output, window->get_title().substr(0, output_size).c_str());
    }

    __export bool window_get_decorations(const saucer::window *window)
    {
        return window->get_decorations();
    }

    __export bool window_get_always_on_top(const saucer::window *window)
    {
        return window->get_always_on_top();
    }

    __export void window_get_size(const saucer::window *window, size_t *width, size_t *height)
    {
        auto [w, h] = window->get_size();
        *width = w;
        *height = h;
    }

    __export void window_get_max_size(const saucer::window *window, size_t *width, size_t *height)
    {
        auto [w, h] = window->get_max_size();
        *width = w;
        *height = h;
    }

    __export void window_get_min_size(const saucer::window *window, size_t *width, size_t *height)
    {
        auto [w, h] = window->get_min_size();
        *width = w;
        *height = h;
    }

    __export void window_get_background_color(const saucer::window *window, size_t *r, size_t *g, size_t *b, size_t *a)
    {
        auto [_r, _g, _b, _a] = window->get_background_color();
        *r = _r;
        *g = _g;
        *b = _b;
        *a = _a;
    }

    __export void window_hide(saucer::window *window)
    {
        window->hide();
    }

    __export void window_show(saucer::window *window)
    {
        window->show();
    }

    __export void window_close(saucer::window *window)
    {
        window->close();
    }

    __export void window_set_resizeable(saucer::window *window, bool enabled)
    {
        window->set_resizeable(enabled);
    }

    __export void window_set_title(saucer::window *window, const char *title)
    {
        window->set_title(title);
    }

    __export void window_set_decorations(saucer::window *window, bool enabled)
    {
        window->set_decorations(enabled);
    }

    __export void window_set_always_on_top(saucer::window *window, bool enabled)
    {
        window->set_always_on_top(enabled);
    }

    __export void window_set_size(saucer::window *window, size_t width, size_t height)
    {
        window->set_size(width, height);
    }

    __export void window_set_max_size(saucer::window *window, size_t width, size_t height)
    {
        window->set_max_size(width, height);
    }

    __export void window_set_min_size(saucer::window *window, size_t width, size_t height)
    {
        window->set_min_size(width, height);
    }

    __export void window_set_background_color(saucer::window *window, size_t r, size_t g, size_t b, size_t a)
    {
        window->set_background_color(r, g, b, a);
    }

    enum window_event
    {
        resize,
        close
    };

    __export void window_clear(saucer::window *window, window_event event)
    {
        window->clear(static_cast<saucer::window_event>(event));
    }

    __export void window_unregister(saucer::window *window, window_event event, size_t id)
    {
        window->unregister(static_cast<saucer::window_event>(event), id);
    }

    __export size_t window_on_resize(saucer::window *window, void (*callback)(size_t, size_t))
    {
        return window->on<saucer::window_event::resize>(callback);
    }

    __export size_t window_on_close(saucer::window *window, bool (*callback)())
    {
        return window->on<saucer::window_event::close>(callback);
    }
}

extern "C"
{
    __export bool webview_get_dev_tools(const saucer::webview *webview)
    {
        return webview->get_dev_tools();
    }

    __export void webview_get_url(const saucer::webview *webview, char *output, size_t output_size)
    {
        strcpy(output, webview->get_url().substr(0, output_size).c_str());
    }

    __export bool webview_get_transparent(const saucer::webview *webview)
    {
        return webview->get_transparent();
    }

    __export bool webview_get_context_menu(const saucer::webview *webview)
    {
        return webview->get_context_menu();
    }

    __export void webview_serve_embedded(saucer::webview *webview, const char *file)
    {
        webview->serve_embedded(file);
    }

    __export void webview_set_dev_tools(saucer::webview *webview, bool enabled)
    {
        webview->set_dev_tools(enabled);
    }

    __export void webview_set_context_menu(saucer::webview *webview, bool enabled)
    {
        webview->set_context_menu(enabled);
    }

    __export void webview_set_url(saucer::webview *webview, const char *url)
    {
        webview->set_url(url);
    }

    [[requires_free]] __export std::pair<std::string, saucer::embedded_file> *embedded_file_new(const std::string &name, const std::string &mime, std::size_t size, const std::uint8_t *data)
    {
        return new std::pair<std::string, saucer::embedded_file>(name, {mime, size, data});
    }

    __export void embedded_file_free(saucer::embedded_file *file)
    {
        delete file;
    }

    __export void webview_embed_files(saucer::webview *webview, std::pair<std::string, saucer::embedded_file> **files, std::size_t file_count)
    {
        std::map<const std::string, const saucer::embedded_file> embedded_files;

        for (auto i = 0u; file_count > i; i++)
        {
            auto &file = files[i];
            embedded_files.emplace(file->first, file->second);
        }

        webview->embed_files(std::move(embedded_files));
    }

    __export void webview_set_transparent(saucer::webview *webview, bool enabled, bool blur = false)
    {
        webview->set_transparent(enabled, blur);
    }

    __export void webview_clear_scripts(saucer::webview *webview)
    {
        webview->clear_scripts();
    }

    __export void webview_clear_embedded(saucer::webview *webview)
    {
        webview->clear_embedded();
    }

    __export void webview_run_java_script(saucer::webview *webview, const char *java_script)
    {
        webview->run_java_script(java_script);
    }

    enum load_time
    {
        ready,
        creation,
    };

    __export void webview_inject(saucer::webview *webview, const char *java_script, load_time load_time)
    {
        webview->inject(java_script, static_cast<saucer::load_time>(load_time));
    }

    enum web_event
    {
        url_changed
    };

    __export void webview_clear(saucer::webview *webview, web_event event)
    {
        webview->clear(static_cast<saucer::web_event>(event));
    }

    __export void webview_unregister(saucer::webview *webview, web_event event, size_t id)
    {
        webview->unregister(static_cast<saucer::web_event>(event), id);
    }

    __export size_t webview_on_url_changed(saucer::webview *webview, void (*callback)(const char *))
    {
        return webview->on<saucer::web_event::url_changed>([callback](const std::string &url) { callback(url.c_str()); });
    }
}

extern "C"
{
    __export saucer::ffi_function_data *ffi_function_data_new(std::size_t id, const char *function, void *data)
    {
        auto *rtn = new saucer::ffi_function_data;
        rtn->function = function;
        rtn->data = data;
        rtn->id = id;

        return rtn;
    }

    __export void ffi_function_data_get_function(saucer::ffi_function_data *data, char *output, std::size_t output_size)
    {
        strcpy(output, data->function.substr(0, output_size).c_str());
    }

    __export void *ffi_function_data_get_data(saucer::ffi_function_data *data)
    {
        return data->data;
    }

    __export std::size_t ffi_function_data_get_id(saucer::ffi_function_data *data)
    {
        return data->id;
    }

    __export saucer::ffi_result_data *ffi_result_data_new(std::size_t id, void *data)
    {
        auto *rtn = new saucer::ffi_result_data;
        rtn->data = data;
        rtn->id = id;

        return rtn;
    }

    __export void *ffi_result_data_get_data(saucer::ffi_result_data *data)
    {
        return data->data;
    }

    __export std::size_t ffi_result_data_get_id(saucer::ffi_result_data *data)
    {
        return data->id;
    }

    __export void ffi_serializer_set_buffer_size(std::size_t size)
    {
        saucer::ffi_serializer::buffer_size = size;
    }

    __export void ffi_serializer_set_init_script(const char *script)
    {
        saucer::ffi_serializer::init_script = script;
    }

    __export void ffi_serializer_set_js_serializer(const char *serializer)
    {
        saucer::ffi_serializer::js_serializer = serializer;
    }

    __export void ffi_serializer_set_parse_callback(saucer::ffi_serializer::parse_callback_t callback)
    {
        saucer::ffi_serializer::parse_callback = callback;
    }
}

extern "C"
{
    [[requires_free]] __export saucer::smartview *smartview_new()
    {
        return new saucer::smartview;
    }

    __export void smartview_free(saucer::smartview *smartview)
    {
        delete smartview;
    }

    __export void smartview_expose(saucer::smartview *smartview, const std::string &name, bool async)
    {
        smartview->expose<saucer::ffi_serializer>(name, nullptr, async);
    }

    // TODO: evals
    // TODO: modules
}