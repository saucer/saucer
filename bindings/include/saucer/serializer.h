#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_serializer;

    struct saucer_parse_result;
    struct saucer_message_data;

    saucer_serializer *saucer_serializer_new();

    void saucer_serializer_set_script(saucer_serializer *, const char *script);
    void saucer_serializer_set_js_serializer(saucer_serializer *, const char *serializer);

    typedef saucer_message_data *(*saucer_serializer_parser)(const char *); // NOLINT
    void saucer_serializer_set_parser(saucer_serializer *, saucer_serializer_parser parser);

    struct saucer_result_data;
    struct saucer_function_data;

    saucer_result_data *saucer_result_data_new(uint64_t id, void *user_data);
    saucer_function_data *saucer_function_data_new(uint64_t id, const char *name, void *user_data);

    void *saucer_result_data_get_user_data(saucer_result_data *);
    void *saucer_function_data_get_user_data(saucer_function_data *);

    struct saucer_parse_result_expected;
    struct saucer_parse_result_unexpected;

    saucer_parse_result_expected *saucer_parse_result_ok(const char *code);

    saucer_parse_result_unexpected *saucer_parse_result_error_serialize();
    saucer_parse_result_unexpected *saucer_parse_result_error_bad_type(size_t index, const char *expected);

    typedef saucer_parse_result *(*saucer_serializer_function)(saucer_function_data *); // NOLINT
    void saucer_serializer_set_function(saucer_serializer *, saucer_serializer_function function);

    typedef void (*saucer_serializer_resolver)(saucer_result_data *); // NOLINT
    void saucer_serializer_set_resolver(saucer_serializer *, saucer_serializer_resolver resolver);

#ifdef __cplusplus
}
#endif
