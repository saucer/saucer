#include "webview.h"

#include "memory.h"

#include "utils.hpp"
#include "smartview.impl.hpp"

#include <cstring>
#include <saucer/smartview.hpp>

extern "C"
{
    bool saucer_webview_dev_tools(saucer_handle *handle)
    {
        return handle->dev_tools();
    }

    char *saucer_webview_url(saucer_handle *handle)
    {
        auto title = handle->url();
        auto *rtn  = reinterpret_cast<char *>(saucer_alloc(title.size() + 1));

        strcpy(rtn, title.c_str());

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

    void saucer_webview_set_url(saucer_handle *handle, const char *url)
    {
        handle->set_url(url);
    }

    void saucer_webview_embed(saucer_handle *handle, saucer_embedded_files *files)
    {
        handle->embed(std::move(*cast(files)));
    }

    void saucer_webview_serve(saucer_handle *handle, const char *file)
    {
        handle->serve(file);
    }

    void saucer_webview_clear_scripts(saucer_handle *handle)
    {
        handle->clear_scripts();
    }

    void saucer_webview_clear_embedded(saucer_handle *handle)
    {
        handle->clear_embedded();
    }

    void saucer_webview_execute(saucer_handle *handle, const char *java_script)
    {
        handle->execute(java_script);
    }

    void saucer_webview_inject(saucer_handle *handle, const char *java_script, saucer_load_time load_time)
    {
        handle->inject(java_script, static_cast<saucer::load_time>(load_time));
    }

    void saucer_webview_clear(saucer_handle *handle, saucer_web_event event)
    {
        handle->clear(static_cast<saucer::web_event>(event));
    }

    void saucer_webview_remove(saucer_handle *handle, saucer_web_event event, uint64_t id)
    {
        handle->remove(static_cast<saucer::web_event>(event), id);
    }

    void saucer_webview_once(saucer_handle *handle, saucer_web_event event, void *callback)
    {
        auto cb = cast_callback{callback, handle};

        switch (event)
        {
        case SAUCER_WEB_EVENT_LOAD_FINISHED:
            return handle->once<saucer::web_event::load_finished>(cb);
        case SAUCER_WEB_EVENT_LOAD_STARTED:
            return handle->once<saucer::web_event::load_started>(cb);
        case SAUCER_WEB_EVENT_URL_CHANGED:
            return handle->once<saucer::web_event::url_changed>(cb);
        case SAUCER_WEB_EVENT_DOM_READY:
            return handle->once<saucer::web_event::dom_ready>(cb);
        }
    }

    uint64_t saucer_webview_on(saucer_handle *handle, saucer_web_event event, void *callback)
    {
        auto cb = cast_callback{callback, handle};

        switch (event)
        {
        case SAUCER_WEB_EVENT_LOAD_FINISHED:
            return handle->on<saucer::web_event::load_finished>(cb);
        case SAUCER_WEB_EVENT_LOAD_STARTED:
            return handle->on<saucer::web_event::load_started>(cb);
        case SAUCER_WEB_EVENT_URL_CHANGED:
            return handle->on<saucer::web_event::url_changed>(cb);
        case SAUCER_WEB_EVENT_DOM_READY:
            return handle->on<saucer::web_event::dom_ready>(cb);
        }
    }
}
