#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

    struct saucer_stash;

    void saucer_stash_free(saucer_stash *);

    size_t saucer_stash_size(saucer_stash *);
    const uint8_t *saucer_stash_data(saucer_stash *);

    saucer_stash *saucer_stash_from(const uint8_t *data, size_t size);
    saucer_stash *saucer_stash_view(const uint8_t *data, size_t size);

    typedef saucer_stash *(*saucer_stash_lazy_callback)();

    /**
     * @note The stash returned from within the @param callback is automatically deleted. However, the stash returned
     * from this function must still be free'd accordingly.
     */
    saucer_stash *saucer_stash_lazy(saucer_stash_lazy_callback callback);

#ifdef __cplusplus
}
#endif
