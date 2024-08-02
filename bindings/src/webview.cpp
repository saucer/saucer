#include "webview.hpp"

#include "icon.hpp"
#include "stash.hpp"
#include "scheme.hpp"
#include "options.hpp"

#include "memory.h"
#include "webview.h"

#include "wrap.hpp"
#include "handle.hpp"

#include <cstring>

struct saucer_embedded_file : bindings::handle<saucer_embedded_file, saucer::embedded_file>
{
};

extern "C"
{
    saucer_embedded_file *saucer_embed(saucer_stash *content, const char *mime)
    {
        return saucer_embedded_file::make(content->value(), mime);
    }

    void saucer_embed_free(saucer_embedded_file *handle)
    {
        delete handle;
    }

    saucer_handle *saucer_new(saucer_options *options)
    {
        return new saucer_handle{options ? options->value() : saucer::options{}};
    }

    void saucer_free(saucer_handle *handle)
    {
        delete handle;
    }

    void saucer_webview_on_message(saucer_handle *handle, saucer_on_message callback)
    {
        handle->m_on_message = callback;
    }

    saucer_icon *saucer_webview_favicon(saucer_handle *handle)
    {
        return saucer_icon::from(handle->favicon());
    }

    void saucer_webview_background(saucer_handle *handle, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
    {
        std::tie(*r, *g, *b, *a) = handle->background();
    }

    char *saucer_webview_page_title(saucer_handle *handle)
    {
        auto title = handle->page_title();

        auto *rtn = static_cast<char *>(saucer_memory_alloc(title.capacity()));
        strncpy(rtn, title.data(), title.capacity());

        return rtn;
    }

    bool saucer_webview_dev_tools(saucer_handle *handle)
    {
        return handle->dev_tools();
    }

    char *saucer_webview_url(saucer_handle *handle)
    {
        auto url = handle->url();

        auto *rtn = static_cast<char *>(saucer_memory_alloc(url.capacity()));
        strncpy(rtn, url.data(), url.capacity());

        return rtn;
    }

    bool saucer_webview_context_menu(saucer_handle *handle)
    {
        return handle->context_menu();
    }

    void saucer_webview_set_dev_tools(saucer_handle *handle, bool enabled)
    {
        handle->set_dev_tools(enabled);
    }

    void saucer_webview_set_context_menu(saucer_handle *handle, bool enabled)
    {
        handle->set_context_menu(enabled);
    }

    void saucer_webview_set_background(saucer_handle *handle, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        handle->set_background({r, g, b, a});
    }

    void saucer_webview_set_file(saucer_handle *handle, const char *file)
    {
        handle->set_file(file);
    }

    void saucer_webview_set_url(saucer_handle *handle, const char *url)
    {
        handle->set_url(url);
    }

    void saucer_webview_embed_file(saucer_handle *handle, const char *name, saucer_embedded_file *file)
    {
        handle->embed({{name, file->value()}});
    }

    void saucer_webview_serve(saucer_handle *handle, const char *file)
    {
        handle->serve(file);
    }

    void saucer_webview_serve_scheme(saucer_handle *handle, const char *file, const char *scheme)
    {
        handle->serve(file, scheme);
    }

    void saucer_webview_clear_scripts(saucer_handle *handle)
    {
        handle->clear_scripts();
    }

    void saucer_webview_clear_embedded(saucer_handle *handle)
    {
        handle->clear_embedded();
    }

    void saucer_webview_clear_embedded_file(saucer_handle *handle, const char *file)
    {
        handle->clear_embedded(file);
    }

    void saucer_webview_execute(saucer_handle *handle, const char *java_script)
    {
        handle->execute(java_script);
    }

    void saucer_webview_inject(saucer_handle *handle, const char *java_script, SAUCER_LOAD_TIME load_time)
    {
        handle->inject(java_script, static_cast<saucer::load_time>(load_time));
    }

    void saucer_webview_handle_scheme(saucer_handle *handle, const char *name, saucer_scheme_handler handler)
    {
        handle->handle_scheme(name,
                              [handle, handler](const saucer::request &request)
                              {
                                  auto *wrapped = saucer_request::from(&request);
                                  auto *ptr     = std::invoke(handler, handle, wrapped);
                                  auto rtn      = ptr->value();

                                  delete ptr;

                                  return rtn;
                              });
    }

    void saucer_webview_remove_scheme(saucer_handle *handle, const char *name)
    {
        handle->remove_scheme(name);
    }

    void saucer_webview_clear(saucer_handle *handle, SAUCER_WEB_EVENT event)
    {
        handle->clear(static_cast<saucer::web_event>(event));
    }

    void saucer_webview_remove(saucer_handle *handle, SAUCER_WEB_EVENT event, uint64_t id)
    {
        handle->remove(static_cast<saucer::web_event>(event), id);
    }

    void saucer_webview_once(saucer_handle *handle, SAUCER_WEB_EVENT event, void *callback)
    {
        using saucer::web_event;
        using events = saucer::webview::events;

        switch (event)
        {
        case SAUCER_WEB_EVENT_URL_CHANGED:
            return handle->once<web_event::url_changed>(
                bindings::callback<events::type<web_event::url_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_ICON_CHANGED:
            return handle->once<web_event::icon_changed>(
                bindings::callback<events::type<web_event::icon_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_TITLE_CHANGED:
            return handle->once<web_event::title_changed>(
                bindings::callback<events::type<web_event::title_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_DOM_READY:
            return handle->once<web_event::dom_ready>(
                bindings::callback<events::type<web_event::dom_ready>>(handle, callback));
        case SAUCER_WEB_EVENT_LOAD_STARTED:
            return handle->once<web_event::load_started>(
                bindings::callback<events::type<web_event::load_started>>(handle, callback));
        case SAUCER_WEB_EVENT_LOAD_FINISHED:
            return handle->once<web_event::load_finished>(
                bindings::callback<events::type<web_event::load_finished>>(handle, callback));
        }
    }

    uint64_t saucer_webview_on(saucer_handle *handle, SAUCER_WEB_EVENT event, void *callback)
    {
        using saucer::web_event;
        using events = saucer::webview::events;

        switch (event)
        {
        case SAUCER_WEB_EVENT_URL_CHANGED:
            return handle->on<web_event::url_changed>(
                bindings::callback<events::type<web_event::url_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_ICON_CHANGED:
            return handle->on<web_event::icon_changed>(
                bindings::callback<events::type<web_event::icon_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_TITLE_CHANGED:
            return handle->on<web_event::title_changed>(
                bindings::callback<events::type<web_event::title_changed>>(handle, callback));
        case SAUCER_WEB_EVENT_DOM_READY:
            return handle->on<web_event::dom_ready>(
                bindings::callback<events::type<web_event::dom_ready>>(handle, callback));
        case SAUCER_WEB_EVENT_LOAD_STARTED:
            return handle->on<web_event::load_started>(
                bindings::callback<events::type<web_event::load_started>>(handle, callback));
        case SAUCER_WEB_EVENT_LOAD_FINISHED:
            return handle->on<web_event::load_finished>(
                bindings::callback<events::type<web_event::load_finished>>(handle, callback));
        }
    }

    void saucer_register_scheme(const char *name)
    {
        saucer::webview::register_scheme(name);
    }
}
