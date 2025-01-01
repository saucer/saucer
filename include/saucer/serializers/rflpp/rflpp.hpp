#pragma once

#include "../generic/generic.hpp"

#include <rfl/json.hpp>

namespace saucer::serializers::rflpp
{
    struct function_data : saucer::function_data
    {
        rfl::Generic params;
    };

    struct result_data : saucer::result_data
    {
        rfl::Generic result;
    };

    class interface
    {
        template <typename T>
        using result = std::expected<T, std::string>;

      public:
        template <typename T>
        static result<T> parse(const auto &);

      public:
        template <typename T>
        static result<T> parse(const result_data &);

        template <typename T>
        static result<T> parse(const function_data &);

      public:
        template <typename T>
        static std::string serialize(T &&);
    };

    struct serializer : generic::serializer<function_data, result_data, interface>
    {
        ~serializer() override;

      public:
        [[nodiscard]] std::string script() const override;
        [[nodiscard]] std::string js_serializer() const override;

      public:
        [[nodiscard]] parse_result parse(const std::string &) const override;
    };
} // namespace saucer::serializers::rflpp

#include "rflpp.inl"
