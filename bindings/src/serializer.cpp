#include "serializer.impl.hpp"

#include <saucer/serializers/errors/bad_type.hpp>
#include <saucer/serializers/errors/serialize.hpp>

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

    saucer_function_data *saucer_function_data_new(uint64_t id, const char *name, void *user_data)
    {
        auto *rtn = new saucer_function_data;

        rtn->id        = id;
        rtn->name      = name;
        rtn->user_data = user_data;

        return rtn;
    }

    void *saucer_result_data_get_user_data(saucer_result_data *handle)
    {
        return handle->user_data;
    }

    void *saucer_function_data_get_user_data(saucer_function_data *handle)
    {
        return handle->user_data;
    }

    saucer_parse_result_expected *saucer_parse_result_ok(const char *code)
    {
        auto *rtn = new saucer_parse_result_expected;
        rtn->code = code;

        return rtn;
    }

    saucer_parse_result_unexpected *saucer_parse_result_error_serialize()
    {
        auto *rtn  = new saucer_parse_result_unexpected;
        rtn->error = std::make_unique<saucer::errors::serialize>();

        return rtn;
    }

    saucer_parse_result_unexpected *saucer_parse_result_error_bad_type(size_t index, const char *expected)
    {
        auto *rtn  = new saucer_parse_result_unexpected;
        rtn->error = std::make_unique<saucer::errors::bad_type>(index, expected);

        return rtn;
    }

    void saucer_serializer_set_function(saucer_serializer *handle, saucer_serializer_function function)
    {
        handle->m_function = function;
    }

    void saucer_serializer_set_resolver(saucer_serializer *handle, saucer_serializer_resolver resolver)
    {
        handle->m_resolver = resolver;
    }
}
