#include "exports.hpp"
#include <cstring>
#include <webview.hpp>

struct embedded_file
{
    const char *file_name;
    const char *mime_type;
    size_t size;
    const unsigned char *data;
};

class saucer::bridged_webview : public saucer::webview
{
  public:
    void (*on_message_callback)(const char *);

  protected:
    void on_message(const std::string &message) override
    {
        webview::on_message(message);
        on_message_callback(message.c_str());
    }
};

void window_show(saucer::window *window)
{
    window->show();
}

void window_hide(saucer::window *window)
{
    window->hide();
}

void window_run()
{
    saucer::window::run();
}

void window_set_title(saucer::window *window, const char *title)
{
    window->set_title(title);
}

void window_set_size(saucer::window *window, size_t width, size_t height)
{
    window->set_size(width, height);
}

void window_set_min_size(saucer::window *window, size_t width, size_t height)
{
    window->set_max_size(width, height);
}

void window_set_max_size(saucer::window *window, size_t width, size_t height)
{
    window->set_max_size(width, height);
}

void window_set_resizeable(saucer::window *window, bool enabled)
{
    window->set_resizeable(enabled);
}

void window_set_decorations(saucer::window *window, bool enabled)
{
    window->set_decorations(enabled);
}

void window_set_always_on_top(saucer::window *window, bool enabled)
{
    window->set_always_on_top(enabled);
}

void window_get_title(saucer::window *window, char *result, unsigned int result_length)
{
    auto title = window->get_title().substr(0, result_length); //? Shorten string to make sure it fits into target.
    strcpy(result, title.c_str());
}

void window_get_size(saucer::window *window, size_t *width, size_t *height)
{
    auto [_width, _height] = window->get_size();
    *width = _width;
    *height = _height;
}

void window_get_min_size(saucer::window *window, size_t *width, size_t *height)
{
    auto [_width, _height] = window->get_min_size();
    *width = _width;
    *height = _height;
}

void window_get_max_size(saucer::window *window, size_t *width, size_t *height)
{
    auto [_width, _height] = window->get_max_size();
    *width = _width;
    *height = _height;
}

bool window_get_resizeable(saucer::window *window)
{
    return window->get_resizeable();
}

bool window_get_decorations(saucer::window *window)
{
    return window->get_decorations();
}

bool window_get_always_on_top(saucer::window *window)
{
    return window->get_always_on_top();
}

void window_on_close(saucer::window *window, bool (*callback)())
{
    window->on_close(callback);
}

void window_on_resize(saucer::window *window, void (*callback)(size_t, size_t))
{
    window->on_resize(callback);
}

saucer::webview *webview_new()
{
    return new saucer::webview;
}

saucer::bridged_webview *bridged_webview_new()
{
    return new saucer::bridged_webview;
}

void webview_free(saucer::webview *webview)
{
    delete webview;
}

void bridged_webview_free(saucer::bridged_webview *webview)
{
    delete webview;
}

bool webview_get_dev_tools(saucer::webview *webview)
{
    return webview->get_dev_tools();
}

bool webview_get_context_menu(saucer::webview *webview)
{
    return webview->get_context_menu();
}

void webview_get_url(saucer::webview *webview, char *result, unsigned int result_length)
{
    auto url = webview->get_url().substr(0, result_length); //? Shorten string to make sure it fits into target.
    strcpy(result, url.c_str());
}

void webview_set_url(saucer::webview *webview, const char *url)
{
    webview->set_url(url);
}

void webview_set_dev_tools(saucer::webview *webview, bool enabled)
{
    webview->set_dev_tools(enabled);
}

void webview_set_context_menu(saucer::webview *webview, bool enabled)
{
    webview->set_context_menu(enabled);
}

void webview_serve_embedded(saucer::webview *webview, const char *file)
{
    webview->serve_embedded(file);
}

embedded_file *saucer_make_embedded_file(const char *file_name, const char *mime_type, size_t size, const unsigned char *data)
{
    return new embedded_file{file_name, mime_type, size, data};
}

void saucer_free_embedded_file(embedded_file *file)
{
    delete file;
}

void webview_run_java_script(saucer::webview *webview, const char *java_script)
{
    webview->run_java_script(java_script);
}

void webview_embed_files(saucer::webview *webview, embedded_file **files, size_t file_count)
{
    std::map<const std::string, std::tuple<std::string, std::size_t, const std::uint8_t *>> embedded_files;

    for (auto i = 0u; file_count > i; i++)
    {
        auto *file = files[i];
        embedded_files.emplace(file->file_name, std::make_tuple(file->mime_type, file->size, file->data));
    }

    webview->embed_files(embedded_files);
}

void webview_inject(saucer::webview *webview, const char *code, char load_time)
{
    webview->inject(code, load_time == 1 ? saucer::load_time_t::creation : saucer::load_time_t::ready);
}

void webview_clear_embedded(saucer::webview *webview)
{
    webview->clear_embedded();
}

void webview_clear_scripts(saucer::webview *webview)
{
    webview->clear_scripts();
}

void webview_on_url_changed(saucer::webview *webview, void (*callback)(const char *))
{
    webview->on_url_changed([callback](const std::string &url) { callback(url.c_str()); });
}

void bridged_webview_on_message(saucer::bridged_webview *webview, void (*callback)(const char *))
{
    webview->on_message_callback = callback;
}
