#include "embed.h"

#include <saucer/webview.hpp>

extern "C"
{
    struct saucer_embedded_files
    {
        saucer::webview::embedded_files files;
    };

    saucer_embedded_files *saucer_embedded_files_new()
    {
        return new saucer_embedded_files;
    }

    void saucer_embedded_files_free(saucer_embedded_files *handle)
    {
        delete handle;
    }

    void saucer_embedded_files_add(saucer_embedded_files *handle, const char *file, const char *mime,
                                   const uint8_t *content)
    {
        handle->files.emplace(file, mime, content);
    }
}
