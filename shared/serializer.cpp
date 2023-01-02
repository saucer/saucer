#include "serializer.hpp"
#include "serializers/serializer.hpp"

ffi_function_data::~ffi_function_data() = default;

ffi_result_data::~ffi_result_data() = default;

serializer::~serializer() = default;

std::string serializer::m_script;
std::string serializer::m_serializer;

std::size_t serializer::buffer_size{2048};
serializer::parse_callback_t serializer::parse_callback;
serializer::serialize_callback_t serializer::serialize_callback;

std::string serializer::init_script() const
{
    return m_script;
}

std::string serializer::js_serializer() const
{
    return m_serializer;
}

std::unique_ptr<saucer::message_data> serializer::parse(const std::string &data) const
{
    return std::unique_ptr<saucer::message_data>(parse_callback(data.c_str()));
}

saucer::serializer::resolve_callback serializer::serialize_function(saucer::smartview *smartview)
{
    return [smartview](saucer::function_data &data) -> tl::expected<std::string, error> {
        error error{};
        std::size_t num_written{};
        auto buffer = std::make_unique<char[]>(buffer_size);

        if (serialize_callback(smartview, &data, buffer.get(), buffer_size, &num_written,
                               reinterpret_cast<int *>(&error)))
        {
            return std::string(buffer.get(), num_written);
        }

        return tl::make_unexpected(error);
    };
}