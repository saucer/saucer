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

    void saucer_icon_from_file(saucer_icon **result, const char *file);
    void saucer_icon_from_data(saucer_icon **result, saucer_stash *stash);

#ifdef __cplusplus
}
#endif
