#include "embed.h"

#include "utils.hpp"
#include <saucer/webview.hpp>

extern "C"
{
    saucer_embedded_files *saucer_embedded_files_new()
    {
        return cast(new saucer::webview::embedded_files);
    }

    void saucer_embedded_files_free(saucer_embedded_files *handle)
    {
        delete cast(handle);
    }

    void saucer_embedded_files_add(saucer_embedded_files *handle, const char *file, const char *mime,
                                   const uint8_t *content, size_t content_size)
    {
        auto item = saucer::embedded_file{mime, {content, content_size}};
        cast(handle)->emplace(file, std::move(item));
    }
}
