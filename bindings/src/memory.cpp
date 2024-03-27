#include "memory.h"

#include <cstdlib>

extern "C"
{
    void saucer_alloc_free(void *ptr)
    {
        free(ptr); // NOLINT
    }

    void *saucer_alloc(size_t size)
    {
        return malloc(size); // NOLINT
    }
}
