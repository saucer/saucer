#pragma once

#include "../serializer.hpp"

#include <expected>

namespace saucer::serializers::generic
{
    template <typename T, typename FunctionData, typename ResultData>
    concept Serializer = requires(FunctionData &function_data, ResultData &result_data) {
        { T::serialize(10) } -> std::same_as<std::string>;
        { T::template parse<int>("") } -> std::same_as<std::expected<int, std::string>>;
        { T::template parse<int>(result_data) } -> std::same_as<std::expected<int, std::string>>;
        { T::template parse<std::tuple<int>>(function_data) } -> std::same_as<std::expected<std::tuple<int>, std::string>>;
    };

    template <typename FunctionData, typename ResultData, Serializer<FunctionData, ResultData> Interface>
    struct serializer : saucer::serializer
    {
        template <typename Function>
        static auto serialize(Function);

        template <typename... Ts>
        static auto serialize_args(Ts &&...);

      public:
        template <typename T>
        static auto resolve(std::promise<T>);
    };
} // namespace saucer::serializers::generic

#include "generic.inl"
