#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    saucer_color *saucer_color_new();
    void saucer_color_free(saucer_color *);

    uint8_t saucer_color_r(saucer_color *);
    uint8_t saucer_color_g(saucer_color *);
    uint8_t saucer_color_b(saucer_color *);
    uint8_t saucer_color_a(saucer_color *);

    void saucer_color_set_r(saucer_color *, uint8_t r);
    void saucer_color_set_g(saucer_color *, uint8_t g);
    void saucer_color_set_b(saucer_color *, uint8_t b);
    void saucer_color_set_a(saucer_color *, uint8_t a);

#ifdef __cplusplus
}
#endif
