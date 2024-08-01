#include "icon.h"
#include "utils.hpp"

#include <saucer/icon.hpp>

struct saucer_icon : saucer::handle<saucer_icon, saucer::icon>
{
};

extern "C"
{
    void saucer_icon_free(saucer_icon *handle)
    {
        delete handle;
    }

    bool saucer_icon_empty(saucer_icon *handle)
    {
        return handle->value().empty();
    }

    void saucer_icon_from_file(saucer_icon **result, const char *file)
    {
        auto icon = saucer::icon::from(file);

        if (!icon)
        {
            return;
        }

        *result = saucer_icon::from(icon.value());
    }

    void saucer_icon_from_data_copy(saucer_icon **result, const uint8_t *data, size_t size)
    {
        auto icon = saucer::icon::from(saucer::stash<>::from({data, data + size}));

        if (!icon)
        {
            return;
        }

        *result = saucer_icon::from(icon.value());
    }

    void saucer_icon_from_data_view(saucer_icon **result, const uint8_t *data, size_t size)
    {
        auto icon = saucer::icon::from(saucer::stash<>::view({data, data + size}));

        if (!icon)
        {
            return;
        }

        *result = saucer_icon::from(icon.value());
    }
}
