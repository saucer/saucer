#pragma once
#include "ffi.hpp"
#include <tl/expected.hpp>

namespace saucer
{
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

            if (serialize_callback(func, data.get(), buffer.get(), buffer_size, &num_written, &error))
            {
                return std::string(buffer.get(), num_written);
            }

            return tl::make_unexpected(error);
        };
    }

    inline std::size_t ffi_serializer::buffer_size = 2048;

    inline std::string ffi_serializer::init_script;
    inline std::string ffi_serializer::js_serializer;

    inline ffi_serializer::parse_callback_t ffi_serializer::parse_callback;
    inline ffi_serializer::serialize_callback_t ffi_serializer::serialize_callback;
} // namespace saucer