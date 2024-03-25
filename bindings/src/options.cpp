#include "options.h"

#include <saucer/window.hpp>

extern "C"
{
    struct saucer_options
    {
        saucer::options options;
    };

    saucer_options *saucer_options_new()
    {
        return new saucer_options;
    }

    void saucer_options_free(saucer_options *handle)
    {
        delete handle;
    }

    void saucer_options_set_persistent_cookies(saucer_options *handle, bool enabled)
    {
        handle->options.persistent_cookies = enabled;
    }

    void saucer_options_set_hardware_acceleration(saucer_options *handle, bool enabled)
    {
        handle->options.hardware_acceleration = enabled;
    }

    void saucer_options_set_storage_path(saucer_options *handle, const char *path)
    {
        handle->options.storage_path = path;
    }

    void saucer_options_add_chrome_flag(saucer_options *handle, const char *flag)
    {
        handle->options.chrome_flags.emplace_back(flag);
    }
}
