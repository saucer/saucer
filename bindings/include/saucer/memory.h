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

    void saucer_alloc_free(void *);
    void *saucer_alloc(size_t size);

#ifdef __cplusplus
}
#endif
