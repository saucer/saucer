#pragma once

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_handle;

    void saucer_free(void *);
    void *saucer_alloc(size_t size);

#ifdef __cplusplus
}
#endif
