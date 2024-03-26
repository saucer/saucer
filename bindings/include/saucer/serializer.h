#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct saucer_serializer;
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

#ifdef __cplusplus
}
#endif
