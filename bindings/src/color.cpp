#include "color.h"

#include "utils.hpp"
#include <saucer/window.hpp>

extern "C"
{
    saucer_color *saucer_color_new()
    {
        return cast(new saucer::color{0, 0, 0, 0});
    }

    void saucer_color_free(saucer_color *handle)
    {
        delete cast(handle);
    }

    uint8_t saucer_color_r(saucer_color *handle)
    {
        return cast(handle)->at(0);
    }

    uint8_t saucer_color_g(saucer_color *handle)
    {
        return cast(handle)->at(1);
    }

    uint8_t saucer_color_b(saucer_color *handle)
    {
        return cast(handle)->at(2);
    }

    uint8_t saucer_color_a(saucer_color *handle)
    {
        return cast(handle)->at(3);
    }

    void saucer_color_set_r(saucer_color *handle, uint8_t r)
    {
        cast(handle)->at(0) = r;
    }

    void saucer_color_set_g(saucer_color *handle, uint8_t g)
    {
        cast(handle)->at(1) = g;
    }

    void saucer_color_set_b(saucer_color *handle, uint8_t b)
    {
        cast(handle)->at(2) = b;
    }

    void saucer_color_set_a(saucer_color *handle, uint8_t a)
    {
        cast(handle)->at(3) = a;
    }
}
