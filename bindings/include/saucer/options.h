#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

    struct saucer_options;

    saucer_options *saucer_options_new();
    void saucer_options_free(saucer_options *);

    void saucer_options_set_persistent_cookies(saucer_options *, bool enabled);
    void saucer_options_set_hardware_acceleration(saucer_options *, bool enabled);

    void saucer_options_set_storage_path(saucer_options *, const char *path);
    void saucer_options_add_chrome_flag(saucer_options *, const char *flag);

#ifdef __cplusplus
}
#endif
