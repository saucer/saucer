#include "exports.hpp"
#include "serializer.hpp"
#include <saucer/smartview.hpp>

struct embedded_file
{
    std::string name;
    saucer::embedded_file file;
};

extern "C"
{
    smartview *smartview_new()
    {
        return new smartview;
    }

    void smartview_free(smartview *smartview)
    {
        delete smartview;
    }

    bool smartview_get_resizable(smartview *smartview)
    {
        return smartview->get_resizable();
    }
    void smartview_get_title(smartview *smartview, char *output, size_t size)
    {
        strncpy(output, smartview->get_title().c_str(), size);
    }
    bool smartview_get_decorations(smartview *smartview)
    {
        return smartview->get_decorations();
    }
    bool smartview_get_always_on_top(smartview *smartview)
    {
        return smartview->get_always_on_top();
    }

    void smartview_get_size(smartview *smartview, size_t *width_o, size_t *height_o)
    {
        auto [width, height] = smartview->get_size();

        *width_o = width;
        *height_o = height;
    }
    void smartview_get_max_size(smartview *smartview, size_t *width_o, size_t *height_o)
    {
        auto [width, height] = smartview->get_max_size();

        *width_o = width;
        *height_o = height;
    }
    void smartview_get_min_size(smartview *smartview, size_t *width_o, size_t *height_o)
    {
        auto [width, height] = smartview->get_min_size();

        *width_o = width;
        *height_o = height;
    }
    void smartview_get_background_color(smartview *smartview, size_t *r_o, size_t *g_o, size_t *b_o, size_t *a_o)
    {
        auto [r, g, b, a] = smartview->get_background_color();

        *r_o = r;
        *g_o = g;
        *b_o = b;
        *a_o = a;
    }

    void smartview_hide(smartview *smartview)
    {
        smartview->hide();
    }
    void smartview_show(smartview *smartview)
    {
        smartview->show();
    }
    void smartview_close(smartview *smartview)
    {
        smartview->close();
    }

    void smartview_set_resizable(smartview *smartview, bool enabled)
    {
        smartview->set_resizable(enabled);
    }
    void smartview_set_title(smartview *smartview, const char *title)
    {
        smartview->set_title(title);
    }
    void smartview_set_decorations(smartview *smartview, bool enabled)
    {
        smartview->set_decorations(enabled);
    }
    void smartview_set_always_on_top(smartview *smartview, bool enabled)
    {
        smartview->set_always_on_top(enabled);
    }

    void smartview_set_size(smartview *smartview, size_t width, size_t height)
    {
        smartview->set_size(width, height);
    }
    void smartview_set_max_size(smartview *smartview, size_t width, size_t height)
    {
        smartview->set_max_size(width, height);
    }
    void smartview_set_min_size(smartview *smartview, size_t width, size_t height)
    {
        smartview->set_min_size(width, height);
    }
    void smartview_set_background_color(smartview *smartview, size_t r, size_t g, size_t b, size_t a)
    {
        smartview->set_background_color(r, g, b, a);
    }

    bool smartview_get_dev_tools(smartview *smartview)
    {
        return smartview->get_dev_tools();
    }
    void smartview_get_url(smartview *smartview, char *output, size_t size)
    {
        strncpy(output, smartview->get_url().c_str(), size);
    }
    bool smartview_get_transparent(smartview *smartview)
    {
        return smartview->get_transparent();
    }
    bool smartview_get_context_menu(smartview *smartview)
    {
        return smartview->get_context_menu();
    }

    void smartview_clear_scripts(smartview *smartview)
    {
        smartview->clear_scripts();
    }
    void smartview_clear_embedded(smartview *smartview)
    {
        smartview->clear_embedded();
    }
    void smartview_run_java_script(smartview *smartview, const char *java_script)
    {
        smartview->run_java_script(java_script);
    }
    void smartview_inject(smartview *smartview, const char *java_script, load_time load_time)
    {
        smartview->inject(java_script, static_cast<saucer::load_time>(load_time));
    }

    void smartview_set_dev_tools(smartview *smartview, bool enabled)
    {
        smartview->set_dev_tools(enabled);
    }
    void smartview_set_context_menu(smartview *smartview, bool enabled)
    {
        smartview->set_context_menu(enabled);
    }
    void smartview_set_url(smartview *smartview, const char *url)
    {
        smartview->set_url(url);
    }
    void smartview_set_transparent(smartview *smartview, bool enabled, bool blur)
    {
        smartview->set_transparent(enabled, blur);
    }

    embedded_file *embedded_file_new(const char *name, const char *mime, size_t size, const uint8_t *data)
    {
        return new embedded_file{name, {mime, size, data}};
    }
    void embedded_file_free(embedded_file *file)
    {
        delete file;
    }

    void smartview_serve_embedded(smartview *smartview, const char *file)
    {
        smartview->serve_embedded(file);
    }
    void smartview_embed_files(smartview *smartview, embedded_file **files, size_t size)
    {
        std::map<const std::string, const saucer::embedded_file> embedded_files;

        for (auto i = 0u; size > i; i++)
        {
            embedded_files.emplace(files[i]->name, files[i]->file);
        }

        smartview->embed_files(std::move(embedded_files));
    }

    void smartview_clear(smartview *smartview, event event)
    {
        if (event > 2)
        {
            smartview->clear(static_cast<saucer::window_event>(event));
        }
        else
        {
            smartview->clear(static_cast<saucer::web_event>(event - 2));
        }
    }
    void smartview_remove(smartview *smartview, event event, uint64_t id)
    {
        if (event > 2)
        {
            smartview->remove(static_cast<saucer::window_event>(event), id);
        }
        else
        {
            smartview->remove(static_cast<saucer::web_event>(event - 2), id);
        }
    }

    uint64_t smartview_on_close(smartview *smartview, bool (*callback)())
    {
        return smartview->on<saucer::window_event::close>(callback);
    }
    uint64_t smartview_on_resize(smartview *smartview, void (*callback)(size_t, size_t))
    {
        return smartview->on<saucer::window_event::resize>(callback);
    }
    uint64_t smartview_on_url_changed(smartview *smartview, void (*callback)(const char *))
    {
        return smartview->on<saucer::web_event::url_changed>([callback](const std::string &url) { callback(url.c_str()); });
    }

    void serializer_set_buffer_size(size_t size)
    {
        serializer::buffer_size = size;
    }

    void serializer_set_init_script(const char *script)
    {
        serializer::init_script = script;
    }

    void serializer_set_js_serializer(const char *js_serializer)
    {
        serializer::js_serializer = js_serializer;
    }

    void serializer_set_parse_callback(message_data *(*callback)(const char *))
    {
        serializer::parse_callback = callback;
    }

    void serializer_set_serializer_callback(bool (*callback)(smartview *, function_data *, char *, size_t, size_t *, int *))
    {
        serializer::serialize_callback = callback;
    }

    function_data *function_data_new(size_t id, const char *function, void *data)
    {
        auto *rtn = new ffi_function_data;

        rtn->id = id;
        rtn->data = data;
        rtn->function = function;

        return rtn;
    }

    void function_data_get_function(function_data *data, char *output, size_t size)
    {
        strncpy(output, data->function.c_str(), size);
    }

    void *function_data_get_data(function_data *data)
    {
        return reinterpret_cast<ffi_function_data *>(data)->data;
    }

    size_t function_data_get_id(function_data *data)
    {
        return data->id;
    }

    void function_data_free(function_data *data)
    {
        delete data;
    }

    result_data *result_data_new(size_t id, void *data)
    {
        auto *rtn = new ffi_result_data;

        rtn->id = id;
        rtn->data = data;

        return rtn;
    }

    void *result_data_get_data(result_data *data)
    {
        return reinterpret_cast<ffi_result_data *>(data)->data;
    }

    size_t result_data_get_id(result_data *data)
    {
        return data->id;
    }

    void result_data_free(result_data *data)
    {
        delete data;
    }

    void smartview_expose(smartview *smartview, const char *name, bool async)
    {
        smartview->expose<serializer>(name, smartview, async);
    }
}