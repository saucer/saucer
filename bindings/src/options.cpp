#include "options.h"

#include "utils.hpp"

extern "C"
{
    saucer_options *saucer_options_new()
    {
        return cast(new saucer::options);
    }

    void saucer_options_free(saucer_options *handle)
    {
        delete cast(handle);
    }

    void saucer_options_set_persistent_cookies(saucer_options *handle, bool enabled)
    {
        cast(handle)->persistent_cookies = enabled;
    }

    void saucer_options_set_hardware_acceleration(saucer_options *handle, bool enabled)
    {
        cast(handle)->hardware_acceleration = enabled;
    }

    void saucer_options_set_storage_path(saucer_options *handle, const char *path)
    {
        cast(handle)->storage_path = path;
    }

    void saucer_options_add_chrome_flag(saucer_options *handle, const char *flag)
    {
        cast(handle)->chrome_flags.emplace_back(flag);
    }
}
