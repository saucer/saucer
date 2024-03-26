#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_embedded_files;

    saucer_embedded_files *saucer_embedded_files_new();
    void saucer_embedded_files_free(saucer_embedded_files *);

    void saucer_embedded_files_add(saucer_embedded_files *, const char *file, const char *mime, const uint8_t *content,
                                   size_t content_size);

#ifdef __cplusplus
}
#endif
