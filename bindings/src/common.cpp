#include "common.h"

#include <cstdlib>

extern "C"
{
    void saucer_free(void *ptr)
    {
        // NOLINTNEXTLINE
        free(ptr);
    }

    void *saucer_new(size_t size)
    {
        // NOLINTNEXTLINE
        return malloc(size);
    }
}
