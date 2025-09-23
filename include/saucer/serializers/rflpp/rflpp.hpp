#pragma once

#include "../serializer.hpp"

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

    template <typename T>
    concept Readable = requires(std::string data) {
        { rfl::json::read<std::remove_cvref_t<T>>(data) };
    };

    template <typename T>
    concept Writable = requires(T value) {
        { rfl::json::write(value) };
    };

    struct serializer : saucer::serializer<serializer>
    {
        using result_data   = rflpp::result_data;
        using function_data = rflpp::function_data;

      public:
        ~serializer() override;

      public:
        [[nodiscard]] std::string script() const override;
        [[nodiscard]] std::string js_serializer() const override;
        [[nodiscard]] parse_result parse(std::string_view) const override;

      public:
        template <Writable T>
        static std::string write(T &&);

        template <Readable T>
        static result<T> read(std::string_view);

      public:
        template <Readable T>
        static result<T> read(const result_data &);

        template <Readable T>
        static result<T> read(const function_data &);
    };
} // namespace saucer::serializers::rflpp

#include "rflpp.inl"
