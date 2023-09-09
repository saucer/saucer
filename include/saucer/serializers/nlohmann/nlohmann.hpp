#pragma once
#include "../serializer.hpp"

#include <future>
#include <nlohmann/json.hpp>

namespace saucer::serializers
{
    struct nlohmann_function_data : public function_data
    {
        nlohmann::json data;
    };

    struct nlohmann_result_data : public result_data
    {
        nlohmann::json data;
    };

    struct [[deprecated]] nlohmann : public serializer
    {
      public:
        ~nlohmann() override;

      public:
        [[nodiscard]] std::string script() const override;
        [[nodiscard]] std::string js_serializer() const override;

      public:
        [[nodiscard]] parse_result parse(const std::string &data) const override;

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

#include "nlohmann.inl"