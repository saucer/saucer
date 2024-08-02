#include "stash.h"
#include "stash.hpp"

extern "C"
{
    void saucer_stash_free(saucer_stash *handle)
    {
        delete handle;
    }

    size_t saucer_stash_size(saucer_stash *handle)
    {
        return handle->value().size();
    }

    const uint8_t *saucer_stash_data(saucer_stash *handle)
    {
        return handle->value().data();
    }

    saucer_stash *saucer_stash_from(const uint8_t *data, size_t size)
    {
        return saucer_stash::from(saucer::stash<>::from({data, data + size}));
    }

    saucer_stash *saucer_stash_view(const uint8_t *data, size_t size)
    {
        return saucer_stash::from(saucer::stash<>::view({data, data + size}));
    }

    saucer_stash *saucer_stash_lazy(saucer_stash_lazy_callback callback)
    {
        return saucer_stash::from(saucer::stash<>::lazy(
            [callback]()
            {
                auto *handle = std::invoke(callback);
                auto rtn     = std::move(handle->value());

                delete handle;

                return rtn;
            }));
    }
}
