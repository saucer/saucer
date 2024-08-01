#pragma once

extern "C"
{
#include <stddef.h>

    void saucer_free(void *data);
    void *saucer_alloc(size_t size);
}
