#include "memory.h"

#include <cstdlib>

extern "C"
{
    void saucer_free(void *data)
    {
        free(data); // NOLINT
    }

    void *saucer_alloc(size_t size)
    {
        return malloc(size); // NOLINT
    }
}
