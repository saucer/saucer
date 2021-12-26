#include "exports.hpp"
#include <smartview.hpp>

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

void window_get_title(saucer::window *window, char *output, size_t output_size)
{
    strcpy(output, window->get_title().substr(0, output_size).c_str());
}

void window_get_size(saucer::window *window, size_t *width, size_t *height)
{
    const auto [_width, _height] = window->get_size();

    *width = _width;
    *height = _height;
}

void window_get_max_size(saucer::window *window, size_t *width, size_t *height)
{
    const auto [_width, _height] = window->get_max_size();

    *width = _width;
    *height = _height;
}

void window_get_min_size(saucer::window *window, size_t *width, size_t *height)
{
    const auto [_width, _height] = window->get_min_size();

    *width = _width;
    *height = _height;
}

void window_hide(saucer::window *window)
{
    window->hide();
}

void window_show(saucer::window *window)
{
    window->show();
}

void window_exit(saucer::window *window)
{
    window->exit();
}

void window_run()
{
    saucer::window::run();
}

void window_on_close(saucer::window *window, bool (*callback)())
{
    window->on_close(callback);
}

void window_on_resize(saucer::window *window, void (*callback)(size_t, size_t))
{
    window->on_resize(callback);
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

void window_set_title(saucer::window *window, const char *title)
{
    window->set_title(title);
}

void window_set_size(saucer::window *window, std::size_t width, std::size_t height)
{
    window->set_size(width, height);
}

void window_set_max_size(saucer::window *window, std::size_t width, std::size_t height)
{
    window->set_max_size(width, height);
}

void window_set_min_size(saucer::window *window, std::size_t width, std::size_t height)
{
    window->set_min_size(width, height);
}

bool webview_get_dev_tools(saucer::webview *webview)
{
    return webview->get_dev_tools();
}

bool webview_get_context_menu(saucer::webview *webview)
{
    return webview->get_context_menu();
}

void webview_get_url(saucer::webview *webview, char *output, size_t output_size)
{
    strcpy(output, webview->get_url().substr(0, output_size).c_str());
}

void webview_set_dev_tools(saucer::webview *webview, bool enabled)
{
    webview->set_dev_tools(enabled);
}

void webview_set_url(saucer::webview *webview, const char *url)
{
    webview->set_url(url);
}

void webview_set_context_menu(saucer::webview *webview, bool enabled)
{
    webview->set_context_menu(enabled);
}

void webview_serve_embedded(saucer::webview *webview, const char *file)
{
    webview->serve_embedded(file);
}

void webview_embed_files(saucer::webview *webview, embedded_file **files, size_t file_size)
{
    std::map<const std::string, std::tuple<std::string, std::size_t, const std::uint8_t *>> embedded_files;
    for (auto i = 0u; file_size > i; i++)
    {
        const auto &file = files[i];
        embedded_files.emplace(file->file_name, std::make_tuple(file->mime_type, file->size, file->data));
    }

    webview->embed_files(embedded_files);
}

void webview_run_java_script(saucer::webview *webview, const char *java_script)
{
    webview->run_java_script(java_script);
}

void webview_inject(saucer::webview *webview, const char *java_script, load_time load_time)
{
    webview->inject(java_script, static_cast<saucer::load_time_t>(load_time));
}

void webview_clear_scripts(saucer::webview *webview)
{
    webview->clear_scripts();
}

void webview_clear_embedded(saucer::webview *webview)
{
    webview->clear_embedded();
}

void webview_on_url_changed(saucer::webview *webview, void (*callback)(const char *))
{
    webview->on_url_changed([callback](const std::string &url) {
        if (callback)
        {
            callback(url.c_str());
        }
    });
}

struct ffi_function_data : public saucer::function_data
{
    void *data;
};

struct ffi_result_data : public saucer::result_data
{
    void *data{};
};

struct ffi_serializer : public saucer::serializer
{
    std::string serializer;
    std::string init_script;
    ffi_parse_callback_t parse_callback;

    std::string initialization_script() const override;
    std::string java_script_serializer() const override;
    std::shared_ptr<saucer::message_data> parse(const std::string &) override;
};

std::string ffi_serializer::initialization_script() const
{
    return init_script;
}

std::string ffi_serializer::java_script_serializer() const
{
    return serializer;
}

std::shared_ptr<saucer::message_data> ffi_serializer::parse(const std::string &data)
{
    if (parse_callback)
    {
        return std::shared_ptr<saucer::message_data>(parse_callback(data.c_str()));
    }
    return nullptr;
}

struct ffi_promise : public saucer::base_promise
{
    using saucer::base_promise::base_promise;

    //? As the following functions are only meant to be used by the user we more or less "discard" them here.
    //? The FFI-Wrapper should only keep an ffi_promise object to receive the rejected callback.
    void wait() override{};
    bool is_ready() override
    {
        return false;
    };
};

struct ffi_smartview : public saucer::smartview
{
  protected:
    void on_message(const std::string &) override;

  public:
    on_message_callback_t on_message_callback{};
    const std::shared_ptr<ffi_serializer> serializer;

  public:
    ffi_smartview();
    ~ffi_smartview() override;
    void reject(const std::size_t &, const char *);
    void resolve(const std::size_t &, const char *);

    void add_callback(const char *, ffi_callback_t, bool);
    void add_eval(ffi_promise *, ffi_resolve_callback_t, const char *);
};

ffi_smartview::ffi_smartview() : serializer(std::make_shared<ffi_serializer>())
{
    m_serializers.emplace(typeid(ffi_serializer), serializer);
}
ffi_smartview::~ffi_smartview() = default;

void ffi_smartview::on_message(const std::string &message)
{
    smartview::on_message(message);
    if (on_message_callback)
    {
        on_message_callback(message.c_str());
    }
}

void ffi_smartview::reject(const std::size_t &id, const char *message)
{
    smartview::reject(id, message);
}

void ffi_smartview::resolve(const std::size_t &id, const char *result)
{
    smartview::reject(id, result);
}

void ffi_smartview::add_callback(const char *function, ffi_callback_t callback, bool async)
{
    auto cpp_callback = [callback](const std::shared_ptr<saucer::message_data> &data) -> tl::expected<std::function<std::string()>, saucer::serializer::error> {
        if (auto ffi_data = std::dynamic_pointer_cast<ffi_function_data>(data); ffi_data)
        {
            ffi_result_callback_t result_callback{};
            auto rtn = callback(ffi_data.get(), &result_callback);

            if (!rtn)
            {
                return tl::make_unexpected(saucer::serializer::error::argument_mismatch);
            }

            assert(((void)("result_callback was not set correctly"), result_callback));
            return [ffi_data, result_callback]() -> std::string {
                char buffer[2048]{""};
                result_callback(ffi_data.get(), buffer, 2048);

                return buffer;
            };
        }

        return tl::make_unexpected(saucer::serializer::error::parser_mismatch);
    };

    smartview::add_callback(serializer, function, cpp_callback, async);
}

void ffi_smartview::add_eval(ffi_promise *promise, ffi_resolve_callback_t callback, const char *code)
{
    auto cpp_callback = [callback](const std::shared_ptr<saucer::result_data> &data) -> tl::expected<void, saucer::serializer::error> {
        if (auto ffi_data = std::dynamic_pointer_cast<ffi_result_data>(data); ffi_data)
        {
            auto result = callback(ffi_data.get());

            if (!result)
            {
                return tl::make_unexpected(saucer::serializer::error::argument_mismatch);
            }

            return {};
        }

        return tl::make_unexpected(saucer::serializer::error::parser_mismatch);
    };

    smartview::add_eval(serializer, std::shared_ptr<saucer::base_promise>(promise), cpp_callback, code);
}

ffi_smartview *smartview_new()
{
    return new ffi_smartview;
}

void smartview_free(ffi_smartview *smartview)
{
    delete smartview;
}

void smartview_add_callback(ffi_smartview *smartview, const char *function, ffi_callback_t callback, bool async)
{
    smartview->add_callback(function, callback, async);
}

void smartview_add_eval(ffi_smartview *smartview, ffi_promise *promise, ffi_resolve_callback_t callback, const char *code)
{
    smartview->add_eval(promise, callback, code);
}

void smartview_reject(ffi_smartview *smartview, size_t id, const char *message)
{
    smartview->reject(id, message);
}

void smartview_resolve(ffi_smartview *smartview, size_t id, const char *result)
{
    smartview->resolve(id, result);
}

void smartview_serializer_set_initialization_script(ffi_smartview *smartview, const char *init_script)
{
    smartview->serializer->init_script = init_script;
}

void smartview_serializer_set_java_script_serializer(ffi_smartview *smartview, const char *serializer)
{
    smartview->serializer->serializer = serializer;
}

void smartview_serializer_set_parse_callback(ffi_smartview *smartview, ffi_parse_callback_t callback)
{
    smartview->serializer->parse_callback = callback;
}

void smartview_set_on_message_callback(ffi_smartview *smartview, on_message_callback_t callback)
{
    smartview->on_message_callback = callback;
}

ffi_result_data *ffi_result_data_new()
{
    return new ffi_result_data;
}
void *ffi_result_data_get_data(ffi_result_data *result_data)
{
    return result_data->data;
}

void ffi_result_data_set_data(ffi_result_data *result_data, void *data)
{
    result_data->data = data;
}

void ffi_result_data_set_id(ffi_result_data *result_data, size_t id)
{
    result_data->id = id;
}

size_t ffi_result_data_get_id(ffi_result_data *result_data)
{
    return result_data->id;
}

ffi_function_data *ffi_function_data_new()
{
    return new ffi_function_data;
}
void *ffi_function_data_get_data(ffi_function_data *function_data)
{
    return function_data->data;
}
void ffi_function_data_set_data(ffi_function_data *function_data, void *data)
{
    function_data->data = data;
}

void ffi_function_data_set_function(ffi_function_data *function_data, const char *function)
{
    function_data->function = function;
}

void ffi_function_data_set_id(ffi_function_data *function_data, size_t id)
{
    function_data->id = id;
}

size_t ffi_function_data_get_id(ffi_function_data *function_data)
{
    return function_data->id;
}

void ffi_function_data_get_function(ffi_function_data *function_data, char *output, size_t output_length)
{
    strcpy(output, function_data->function.substr(0, output_length).c_str());
}

ffi_promise *ffi_promise_new()
{
    return new ffi_promise(std::this_thread::get_id()); //? The thread id doesn't matter here.
}

void ffi_promise_set_fail_callback(ffi_promise *promise, void (*callback)())
{
    promise->fail(callback);
}