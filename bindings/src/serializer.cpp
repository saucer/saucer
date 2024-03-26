#include "serializer.impl.hpp"

extern "C"
{
    saucer_serializer *saucer_serializer_new()
    {
        return new saucer_serializer;
    }

    void saucer_serializer_set_script(saucer_serializer *handle, const char *script)
    {
        handle->m_script = script;
    }

    void saucer_serializer_set_js_serializer(saucer_serializer *handle, const char *serializer)
    {
        handle->m_js_serializer = serializer;
    }

    void saucer_serializer_set_parser(saucer_serializer *handle, saucer_serializer_parser parser)
    {
        handle->m_parser = parser;
    }

    saucer_result_data *saucer_result_data_new(uint64_t id, void *user_data)
    {
        auto *rtn = new saucer_result_data;

        rtn->id        = id;
        rtn->user_data = user_data;

        return rtn;
    }

    saucer_function_data *saucer_result_function_new(uint64_t id, const char *name, void *user_data)
    {
        auto *rtn = new saucer_function_data;

        rtn->id        = id;
        rtn->name      = name;
        rtn->user_data = user_data;

        return rtn;
    }
}
