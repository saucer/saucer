#pragma once
#include <tl/expected.hpp>
#include <saucer/smartview.hpp>
#include <saucer/serializers/serializer.hpp>

#include "ffi.hpp"

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

    ffi_function_data::~ffi_function_data() = default;

    ffi_result_data::~ffi_result_data() = default;

    ffi_serializer::~ffi_serializer() = default;

    std::string ffi_serializer::initialization_script() const
    {
        return init_script;
    }

    std::string ffi_serializer::java_script_serializer() const
    {
        return js_serializer;
    }

    std::shared_ptr<saucer::message_data> ffi_serializer::parse(const std::string &data) const
    {
        return std::shared_ptr<saucer::message_data>(parse_callback(data.c_str()));
    }

    template <typename Function> auto ffi_serializer::serialize_function(const Function &func)
    {
        //? Func is equal to the smartview used, so that the ffi-bindings can properly correlate which smartview instance to pass the call onto.
        return [func](const std::shared_ptr<function_data> &data) -> tl::expected<std::string, error> {
            error error{};
            std::size_t num_written{};
            auto buffer = std::make_unique<char[]>(buffer_size);

            if (serialize_callback(func, data.get(), buffer.get(), buffer_size, &num_written, reinterpret_cast<saucer::error *>(&error)))
            {
                return std::string(buffer.get(), num_written);
            }

            return tl::make_unexpected(error);
        };
    }

    inline std::size_t ffi_serializer::buffer_size = 2048;

    inline std::string ffi_serializer::init_script;
    inline std::string ffi_serializer::js_serializer;

    inline parse_callback_t ffi_serializer::parse_callback;
    inline serialize_callback_t ffi_serializer::serialize_callback;
} // namespace saucer