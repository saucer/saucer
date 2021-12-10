#pragma once
#include <nlohmann/json.hpp>
#include <serializer.hpp>

namespace saucer
{
    namespace serializers
    {
        struct json_data : public message_data
        {
            nlohmann::json data;
            ~json_data() override;
        };

        struct json_serializer : public serializer
        {
          public:
            ~json_serializer() override;
            std::string java_script_serializer() const override;
            std::shared_ptr<message_data> parse(const std::string &data) override;
            template <typename func_t> static auto encode_function(const func_t &func);
        };
    } // namespace serializers
} // namespace saucer

#include "json.inl"