#pragma once
#include "data.hpp"
#include "args.hpp"

#include <string>
#include <memory>
#include <future>

#include <concepts>
#include <functional>

#include <fmt/core.h>
#include <tl/expected.hpp>

namespace saucer
{
    enum class serializer_error
    {
        argument_count_mismatch,
        type_mismatch,
    };

    struct serializer
    {
        using promise = std::function<void(result_data &)>;
        using function = std::function<tl::expected<std::string, serializer_error>(function_data &)>;

      public:
        virtual ~serializer() = default;

      public:
        [[nodiscard]] virtual std::string script() const = 0;
        [[nodiscard]] virtual std::string js_serializer() const = 0;

      public:
        [[nodiscard]] virtual std::unique_ptr<message_data> parse(const std::string &) const = 0;
    };

    template <class T>
    concept Serializer = requires() {
        requires std::movable<T>;
        requires std::derived_from<T, serializer>;
        {
            T::serialize([](int) { return 5; })
        } -> std::convertible_to<serializer::function>;
        {
            T::serialize_args(10, 15, 20)
        } -> std::convertible_to<fmt::dynamic_format_arg_store<fmt::format_context>>;
        {
            T::serialize_args(make_args(10, 15, 20))
        } -> std::convertible_to<fmt::dynamic_format_arg_store<fmt::format_context>>;
        {
            T::resolve(std::declval<std::shared_ptr<std::promise<int>>>())
        } -> std::convertible_to<serializer::promise>;
    };
} // namespace saucer