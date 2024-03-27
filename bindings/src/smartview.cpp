#include "smartview.h"

#include "utils.hpp"
#include "smartview.impl.hpp"

extern "C"
{
    saucer_handle *saucer_new(saucer_serializer *serializer, saucer_options *options)
    {
        auto opts = options ? *cast(options) : saucer::options{};
        return new saucer_handle{serializer, opts};
    }

    void saucer_free(saucer_handle *handle)
    {
        delete handle;
    }

    void saucer_add_function(saucer_handle *handle, const char *name, bool async)
    {
        handle->add_function(name, handle->m_function, async);
    }

    void saucer_add_evaluation(saucer_handle *handle, const char *code)
    {
        handle->add_evaluation(handle->m_resolver, code);
    }
}
