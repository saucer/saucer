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

    template <typename T>
    concept Readable = glz::read_supported<T, glz::JSON>;

    template <typename T>
    concept Writable = glz::write_supported<T, glz::JSON>;

    struct serializer : saucer::serializer<serializer>
    {
        using result_data   = glaze::result_data;
        using function_data = glaze::function_data;

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
} // namespace saucer::serializers::glaze

#include "glaze.inl"
