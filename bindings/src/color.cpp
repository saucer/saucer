#include "color.h"

extern "C"
{
    saucer_color *saucer_color_new()
    {
        return new saucer_color;
    }

    void saucer_color_free(saucer_color *handle)
    {
        delete handle;
    }

    uint8_t saucer_color_r(saucer_color *handle)
    {
        return handle->r;
    }

    uint8_t saucer_color_g(saucer_color *handle)
    {
        return handle->g;
    }

    uint8_t saucer_color_b(saucer_color *handle)
    {
        return handle->b;
    }

    uint8_t saucer_color_a(saucer_color *handle)
    {
        return handle->a;
    }

    void saucer_color_set_r(saucer_color *handle, uint8_t r)
    {
        handle->r = r;
    }

    void saucer_color_set_g(saucer_color *handle, uint8_t g)
    {
        handle->g = g;
    }

    void saucer_color_set_b(saucer_color *handle, uint8_t b)
    {
        handle->b = b;
    }

    void saucer_color_set_a(saucer_color *handle, uint8_t a)
    {
        handle->a = a;
    }
}
