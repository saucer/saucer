#pragma once

#include "generic.hpp"

#include "../../utils/tuple.hpp"
#include "../../utils/traits.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

namespace saucer::serializers::generic
{
    namespace impl
    {
        template <typename Interface, typename T>
        std::expected<T, std::string> parse(const auto &data)
        {
            return Interface::template parse<T>(data);
        }

        template <typename Interface, tuple::Tuple T>
            requires(std::tuple_size_v<T> == 0)
        std::expected<T, std::string> parse(const auto &)
        {
            return {};
        }

        template <typename Interface>
        constexpr auto serialize()
        {
            return "";
        }

        template <typename Interface, typename T>
        auto serialize(T &&data)
        {
            return Interface::serialize(std::forward<T>(data));
        }

        template <typename Interface, Arguments T>
        auto serialize(T &&data)
        {
            std::vector<std::string> rtn;
            rtn.reserve(data.size());

            auto unpack = [&]<typename... Ts>(Ts &&...args)
            {
                (rtn.emplace_back(serialize<Interface>(std::forward<Ts>(args))), ...);
            };
            std::apply(unpack, data.as_tuple());

            return fmt::format("{}", fmt::join(rtn, ", "));
        }
    } // namespace impl

    template <typename FunctionData, typename ResultData, Serializer<FunctionData, ResultData> Interface>
    template <typename Function>
    auto serializer<FunctionData, ResultData, Interface>::serialize(Function &&func)
    {
        using resolver  = traits::resolver<Function>;
        using converter = resolver::converter;
        using args      = resolver::args;

        return [func = converter::convert(std::forward<Function>(func))](std::unique_ptr<saucer::function_data> data,
                                                                         serializer::executor exec) mutable
        {
            const auto &message = *static_cast<FunctionData *>(data.get());
            const auto parsed   = impl::parse<Interface, args>(message);

            if (!parsed)
            {
                return std::invoke(exec.reject, impl::serialize<Interface>(parsed.error()));
            }

            auto resolve = [resolve = std::move(exec.resolve)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(resolve, impl::serialize<Interface>(std::forward<Ts>(value)...));
            };

            auto reject = [reject = std::move(exec.reject)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(reject, impl::serialize<Interface>(std::forward<Ts>(value)...));
            };

            auto executor = typename resolver::executor{std::move(resolve), std::move(reject)};
            auto params   = std::tuple_cat(std::move(parsed.value()), std::make_tuple(std::move(executor)));

            std::apply(func, std::move(params));
        };
    }

    template <typename FunctionData, typename ResultData, Serializer<FunctionData, ResultData> Interface>
    template <typename... Ts>
    auto serializer<FunctionData, ResultData, Interface>::serialize_args(Ts &&...params)
    {
        serializer::args rtn;

        rtn.reserve(sizeof...(params), 0);
        (rtn.push_back(impl::serialize<Interface>(std::forward<Ts>(params))), ...);

        return rtn;
    }

    template <typename FunctionData, typename ResultData, Serializer<FunctionData, ResultData> Interface>
    template <typename T>
    auto serializer<FunctionData, ResultData, Interface>::resolve(std::promise<T> promise)
    {
        return [promise = std::move(promise)](std::unique_ptr<saucer::result_data> data) mutable
        {
            const auto &res = *static_cast<ResultData *>(data.get());

            if constexpr (!std::is_void_v<T>)
            {
                auto parsed = impl::parse<Interface, T>(res);

                if (!parsed)
                {
                    auto exception = std::runtime_error{parsed.error()};
                    auto ptr       = std::make_exception_ptr(exception);

                    promise.set_exception(ptr);
                    return;
                }

                promise.set_value(parsed.value());
            }
            else
            {
                promise.set_value();
            }
        };
    }
} // namespace saucer::serializers::generic
