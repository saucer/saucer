#pragma once
#include <saucer/serializers/serializer.hpp>

namespace saucer
{
    struct ffi_function_data : public function_data
    {
        void *data;
        ~ffi_function_data() override;
    };

    struct ffi_result_data : public result_data
    {
        void *data;
        ~ffi_result_data() override;
    };

    struct ffi_serializer : public serializer
    {
        using parse_callback_t = message_data *(*)(const char *);
        using serialize_callback_t = bool (*)(function_data *, char *, std::size_t, error *);

      public:
        ~ffi_serializer() override;

      public:
        std::string initialization_script() const override;
        std::string java_script_serializer() const override;
        std::shared_ptr<message_data> parse(const std::string &data) const override;

      public:
        template <typename Function> static auto serialize_function(const Function &func);

      public:
        static std::size_t buffer_size;
        static std::string init_script;
        static std::string js_serializer;
        static parse_callback_t parse_callback;
        static serialize_callback_t serialize_callback;
    };
} // namespace saucer

#include "ffi.inl"