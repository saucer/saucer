#pragma once

#include "options.h"
#include "serializer.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_handle;

    saucer_handle *saucer_new(saucer_serializer *, saucer_options *);
    void saucer_free(saucer_handle *);

    void saucer_add_function(saucer_handle *, const char *name, bool async);
    void saucer_add_evaluation(saucer_handle *, const char *code);

#ifdef __cplusplus
}
#endif
