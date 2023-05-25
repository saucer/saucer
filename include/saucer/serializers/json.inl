#pragma once
#include "json.hpp"

#include "args.hpp"
#include "serializer.hpp"
#include "../smartview.hpp"

#include <variant>
#include <fmt/core.h>
#include <fmt/args.h>
#include <type_traits>
#include <boost/callable_traits.hpp>

namespace saucer::serializers::detail::json
{
    template <typename T>
    concept is_serializable = requires(T t) { static_cast<nlohmann::json>(t); };

    template <typename T> struct decay_tuple
    {
    };

    template <typename... T> struct decay_tuple<std::tuple<T...>>
    {
        using type = std::tuple<std::decay_t<T>...>;
        static constexpr bool serializable = (is_serializable<std::decay_t<T>> && ...);
    };

    template <typename T> using decay_tuple_t = typename decay_tuple<T>::type;
}; // namespace saucer::serializers::detail::json

namespace saucer::serializers
{
    template <typename Function> auto json::serialize(const Function &func)
    {
        using raw_args_t = boost::callable_traits::args_t<Function>;

        using rtn_t = boost::callable_traits::return_type_t<Function>;
        using args_t = detail::json::decay_tuple_t<raw_args_t>;

        static_assert(detail::json::decay_tuple<raw_args_t>::serializable &&
                          (std::is_same_v<rtn_t, void> || detail::json::is_serializable<rtn_t>),
                      "All arguments as well as the return type must be serializable");

        return [func](function_data &data) -> function_serializer::result_type {
            auto &message = dynamic_cast<json_function_data &>(data);
            const auto &params = message.data;

            if (params.size() != std::tuple_size_v<args_t>)
            {
                return tl::make_unexpected(serializer_error::argument_count_mismatch);
            }

            args_t args;

            try
            {
                std::size_t current_arg{0};
                auto serialize_item = [&]<typename T>(T &arg) { arg = static_cast<T>(params.at(current_arg++)); };

                std::apply([&](auto &...args) { (serialize_item(args), ...); }, args);
            }
            catch (...)
            {
                return tl::make_unexpected(serializer_error::type_mismatch);
            }

            nlohmann::json rtn;

            if constexpr (std::is_void_v<rtn_t>)
            {
                std::apply(func, args);
            }
            else
            {
                rtn = std::apply(func, args);
            }

            // ? We dump twice to properly escape
            return fmt::format("JSON.parse({})", static_cast<nlohmann::json>(nlohmann::json(rtn).dump()).dump());
        };
    }

    template <typename... Params> auto json::serialize_args(const Params &...params)
    {
        fmt::dynamic_format_arg_store<fmt::format_context> rtn;

        auto unpack = [&]<typename T>(const T &arg) {
            if constexpr (is_arguments<std::decay_t<T>>)
            {
                static_assert((detail::json::decay_tuple<typename T::tuple_t>::serializable),
                              "All arguments must be serializable");

                std::string rtn;
                auto tuple = static_cast<typename T::tuple_t>(arg);

                auto serialize_item = [&](const auto &item) {
                    auto json = static_cast<nlohmann::json>(nlohmann::json(item).dump()).dump();
                    rtn += fmt::format("JSON.parse({}),", json);
                };

                std::apply([&](const auto &...arg) { (serialize_item(arg), ...); }, tuple);

                if (!rtn.empty())
                {
                    rtn.pop_back();
                }

                return rtn;
            }
            else
            {
                static_assert(detail::json::is_serializable<T>, "All arguments must be serializable");

                auto json = static_cast<nlohmann::json>(nlohmann::json(arg).dump()).dump();
                return fmt::format("JSON.parse({})", json);
            }
        };

        (rtn.push_back(unpack(params)), ...);
        return rtn;
    }

    template <typename T> auto json::resolve(std::shared_ptr<std::promise<T>> promise)
    {
        static_assert((std::is_same_v<T, void> || detail::json::is_serializable<T>),
                      "The promise result must be serializable");

        return [promise = std::move(promise)](result_data &data) mutable {
            auto &json_data = dynamic_cast<json_result_data &>(data);

            if constexpr (std::is_same_v<T, void>)
            {
                promise->set_value();
            }
            else
            {
                try
                {
                    promise->set_value(json_data.data);
                }
                catch (...)
                {
                    promise->set_exception(std::current_exception());
                }
            }
        };
    }
} // namespace saucer::serializers