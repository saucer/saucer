#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

    struct saucer_icon;

    void saucer_icon_free(saucer_icon *);
    bool saucer_icon_empty(saucer_icon *);

    /**
     * The following functions can be used to construct an icon from given data or a file.
     * The pointer given through @param result will be set to the created icon in case the creation was successful.
     */

    void saucer_icon_from_file(saucer_icon **result, const char *file);

    void saucer_icon_from_data_copy(saucer_icon **result, const uint8_t *data, size_t size);
    void saucer_icon_from_data_view(saucer_icon **result, const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif
