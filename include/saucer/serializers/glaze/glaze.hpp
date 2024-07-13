#pragma once

#include "../serializer.hpp"

#include <glaze/glaze.hpp>

namespace saucer::serializers::glaze
{
    struct function_data : saucer::function_data
    {
        glz::raw_json params;
    };

    struct result_data : saucer::result_data
    {
        glz::raw_json result;
    };

    struct serializer : saucer::serializer
    {
        ~serializer() override;

      public:
        [[nodiscard]] std::string script() const override;
        [[nodiscard]] std::string js_serializer() const override;

      public:
        [[nodiscard]] parse_result parse(const std::string &data) const override;

      public:
        template <launch Policy, typename Function>
        static auto serialize(const Function &func);

        template <typename... Ts>
        static auto serialize_args(const Ts &...params);

      public:
        template <typename T>
        static auto resolve(std::shared_ptr<std::promise<T>> promise);
    };
} // namespace saucer::serializers::glaze

#include "glaze.inl"
