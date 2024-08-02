#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "stash.h"
#include <stdbool.h>

    struct saucer_icon;

    void saucer_icon_free(saucer_icon *);
    bool saucer_icon_empty(saucer_icon *);

    /**
     * @brief Try to construct an icon from a given file.
     * @note The pointer pointed to by @param result will be set to a saucer_icon in case of success. The returned icon
     * must be free'd.
     */
    void saucer_icon_from_file(saucer_icon **result, const char *file);

    /**
     * @brief Try to construct an icon from a given stash (raw bytes).
     * @note The pointer pointed to by @param result will be set to a saucer_icon in case of success. The returned icon
     * must be free'd.
     */
    void saucer_icon_from_data(saucer_icon **result, saucer_stash *stash);

#ifdef __cplusplus
}
#endif
