#pragma once
#include <nlohmann/json.hpp>
#include <promise/promise.hpp>
#include <serializers/serializer.hpp>

namespace saucer
{
    namespace serializers
    {
        struct json_function_data : public function_data
        {
            nlohmann::json data;
            ~json_function_data() override;
        };

        struct json_result_data : public result_data
        {
            nlohmann::json data;
            ~json_result_data() override;
        };

        struct json : public serializer
        {
          public:
            ~json() override;
            std::string initialization_script() const override;
            std::string java_script_serializer() const override;
            std::shared_ptr<message_data> parse(const std::string &data) override;

            template <typename func_t> static auto serialize_function(const func_t &func);
            template <typename... params_t> static auto serialize_arguments(const params_t &...params);
            template <typename T> static auto resolve_function(const std::shared_ptr<promise<T>> &promise);
        };
    } // namespace serializers
} // namespace saucer

#include "json.inl"