#include "options.h"
#include "utils.hpp"

#include <saucer/window.hpp>

struct saucer_options : saucer::handle<saucer_options, saucer::options>
{
};

extern "C"
{
    saucer_options *saucer_options_new()
    {
        return saucer_options::make();
    }

    void saucer_options_free(saucer_options *handle)
    {
        delete handle;
    }

    void saucer_options_set_persistent_cookies(saucer_options *handle, bool enabled)
    {
        handle->value().persistent_cookies = enabled;
    }

    void saucer_options_set_hardware_acceleration(saucer_options *handle, bool enabled)
    {
        handle->value().hardware_acceleration = enabled;
    }

    void saucer_options_set_storage_path(saucer_options *handle, const char *path)
    {
        handle->value().storage_path = path;
    }

    void saucer_options_add_chrome_flag(saucer_options *handle, const char *flag)
    {
        handle->value().chrome_flags.emplace(flag);
    }

    void saucer_options_set_threads(saucer_options *handle, size_t count)
    {
        handle->value().threads = count;
    }
}
