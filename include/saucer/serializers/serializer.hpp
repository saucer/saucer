#pragma once

#include "../error.hpp"
#include "../executor.hpp"

#include "data.hpp"
#include "args/args.hpp"

#include <concepts>
#include <functional>

#include <string>
#include <memory>
#include <future>

#include <fmt/args.h>

namespace saucer
{
    struct serializer
    {
        using parse_result = std::unique_ptr<message_data>;
        using executor     = impl::executor<std::string, error>;
        using args         = fmt::dynamic_format_arg_store<fmt::format_context>;

      public:
        using resolver = std::function<void(std::unique_ptr<message_data>)>;
        using function = std::function<void(std::unique_ptr<message_data>, const executor &)>;

      public:
        virtual ~serializer() = default;

      public:
        [[nodiscard]] virtual std::string script() const        = 0;
        [[nodiscard]] virtual std::string js_serializer() const = 0;

      public:
        [[nodiscard]] virtual parse_result parse(const std::string &) const = 0;
    };

    template <class T>
    concept Serializer = requires {
        requires std::movable<T>;
        requires std::derived_from<T, serializer>;
        { //
            T::serialize(std::function<int()>{})
        } -> std::convertible_to<serializer::function>;
        { //
            T::serialize(std::function<void(executor<int>)>{})
        } -> std::convertible_to<serializer::function>;
        { //
            T::serialize_args(10, 15, 20)
        } -> std::convertible_to<serializer::args>;
        { //
            T::serialize_args(make_args(10, 15, 20))
        } -> std::convertible_to<serializer::args>;
        { //
            T::resolve(std::declval<std::shared_ptr<std::promise<int>>>())
        } -> std::convertible_to<serializer::resolver>;
    };
} // namespace saucer
