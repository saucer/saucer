#pragma once
#include <nlohmann/json.hpp>
#include "promise/promise.hpp"
#include "variable/variable.hpp"
#include "serializers/serializer.hpp"

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

        struct json_variable_data : public variable_data
        {
            nlohmann::json data;
            ~json_variable_data() override = default;
        };

        struct json : public serializer
        {
          public:
            ~json() override;
            std::string initialization_script() const override;
            std::string java_script_serializer() const override;
            std::shared_ptr<message_data> parse(const std::string &data) const override;

            template <typename T> static auto serialize_variable(variable<T> &variable);
            template <typename Function> static auto serialize_function(const Function &func);
            template <typename... Params> static auto serialize_arguments(const Params &...params);
            template <typename T> static auto resolve_promise(const std::shared_ptr<promise<T>> &promise);
        };
    } // namespace serializers
} // namespace saucer

#include "json.inl"