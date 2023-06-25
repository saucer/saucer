#pragma once
#include "serializer.hpp"

#include <future>
#include <nlohmann/json.hpp>

namespace saucer::serializers
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
        [[nodiscard]] std::string script() const override;
        [[nodiscard]] std::string js_serializer() const override;

      public:
        [[nodiscard]] message_ptr parse(const std::string &data) const override;

      public:
        template <typename Function>
        static auto serialize(const Function &func);

        template <typename... Params>
        static auto serialize_args(const Params &...params);

      public:
        template <typename T>
        static auto resolve(std::shared_ptr<std::promise<T>> promise);
    };
} // namespace saucer::serializers

#include "json.inl"