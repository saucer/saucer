#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

    void saucer_memory_free(void *data);
    void *saucer_memory_alloc(size_t size);

#ifdef __cplusplus
}
#endif
