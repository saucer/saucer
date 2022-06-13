#pragma once
#include "serializer.hpp"
#include "../promise/promise.hpp"

#include <nlohmann/json.hpp>

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

          public:
            std::string initialization_script() const override;
            std::string java_script_serializer() const override;
            std::shared_ptr<message_data> parse(const std::string &data) const override;

          public:
            template <typename Function> static auto serialize_function(const Function &func);
            template <typename... Params> static auto serialize_arguments(const Params &...params);
            template <typename T> static auto resolve_promise(const std::shared_ptr<promise<T>> &promise);
        };
    } // namespace serializers
} // namespace saucer

#include "json.inl"