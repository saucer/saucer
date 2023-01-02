#pragma once
#include "json.hpp"
#include "../smartview.hpp"

#include <variant>
#include <fmt/format.h>
#include <boost/callable_traits.hpp>

namespace saucer::serializers
{
    template <typename T> struct decay_tuple
    {
    };

    template <typename... T> struct decay_tuple<std::tuple<T...>>
    {
        using type = std::tuple<std::remove_cv_t<std::remove_reference_t<T>>...>;
    };

    template <typename T> using decay_tuple_t = typename decay_tuple<T>::type;

    template <typename Function> auto json::serialize(const Function &func)
    {
        return [func](const std::shared_ptr<function_data> &data) -> tl::expected<std::string, error> {
            using rtn_t = boost::callable_traits::return_type_t<Function>;
            using args_t = decay_tuple_t<boost::callable_traits::args_t<Function>>;

            auto message = std::dynamic_pointer_cast<json_function_data>(data);
            const auto &params = message->data;

            if (params.size() != std::tuple_size_v<args_t>)
            {
                return tl::make_unexpected(error::argument_count_mismatch);
            }

            args_t args;

            try
            {
                std::size_t current_arg{0};

                auto serialize_item = [&](auto &arg) {
                    using arg_t = std::decay_t<decltype(arg)>;
                    arg = static_cast<arg_t>(params.at(current_arg++));
                };

                std::apply([&](auto &...args) { (serialize_item(args), ...); }, args);
            }
            catch (...)
            {
                return tl::make_unexpected(error::type_mismatch);
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
            return fmt::format("JSON.parse({})", nlohmann::json(nlohmann::json(rtn).dump()).dump());
        };
    }

    template <typename... Params> auto json::serialize_args(const Params &...params)
    {
        fmt::dynamic_format_arg_store<fmt::format_context> rtn;

        auto unpack = [&](const auto &arg) {
            using arg_t = std::decay_t<decltype(arg)>;

            if constexpr (is_args_v<arg_t>)
            {
                std::string rtn;
                auto tuple = static_cast<args_t<arg_t>>(arg);

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
                auto json = static_cast<nlohmann::json>(nlohmann::json(arg).dump()).dump();
                return fmt::format(fmt::format("JSON.parse({})", json));
            }
        };

        (rtn.push_back(unpack(params)), ...);
        return rtn;
    }

    template <typename T> auto json::resolve(std::shared_ptr<std::promise<T>> promise)
    {
        return [promise = std::move(promise)](const std::shared_ptr<result_data> &data) mutable {
            auto json_data = std::dynamic_pointer_cast<json_result_data>(data);

            if constexpr (std::is_same_v<T, void>)
            {
                promise->set_value();
            }
            else
            {
                try
                {
                    promise->set_value(json_data->data);
                }
                catch (...)
                {
                    promise->set_exception(std::current_exception());
                }
            }
        };
    }
} // namespace saucer::serializers