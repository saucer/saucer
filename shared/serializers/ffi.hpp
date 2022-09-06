#pragma once
#include <cstdint>

namespace saucer
{
    struct ffi_function_data;
    struct ffi_result_data;
    struct function_data;
    struct message_data;
    struct smartview;

    struct ffi_serializer;

    enum class error
    {
        argument_count_mismatch,
        type_mismatch,
    };

    using parse_callback_t = message_data *(*)(const char *);
    using serialize_callback_t = bool (*)(smartview *, function_data *, char *, std::size_t, std::size_t *, error *);
} // namespace saucer