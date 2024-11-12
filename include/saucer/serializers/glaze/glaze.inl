#pragma once

#include "glaze.hpp"
#include "../../utils/traits.hpp"

#include <optional>
#include <expected>

#include <fmt/core.h>
#include <fmt/xchar.h>

#include <rebind/name.hpp>
#include <rebind/utils/enum.hpp>

namespace saucer::serializers::glaze
{
    namespace impl
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true};

        template <typename T>
        struct serializable_impl : std::false_type
        {
        };

        template <>
        struct serializable_impl<void> : std::true_type
        {
        };

        template <>
        struct serializable_impl<std::tuple<>> : std::true_type
        {
        };

        template <typename T>
            requires glz::read_supported<opts.format, T>
        struct serializable_impl<T> : std::true_type
        {
        };

        template <Arguments T>
        struct serializable_impl<T> : serializable_impl<typename T::tuple>
        {
        };

        template <typename T>
        concept Serializable = serializable_impl<T>::value;

        template <typename T>
        struct is_tuple_impl : std::false_type
        {
        };

        template <typename... Ts>
        struct is_tuple_impl<std::tuple<Ts...>> : std::true_type
        {
        };

        template <typename T>
        concept Tuple = is_tuple_impl<T>::value;

        template <typename T>
            requires(not Tuple<T>)
        std::optional<std::string> mismatch(T &value, const glz::json_t &json)
        {
            auto serialized = glz::write<opts>(json).value_or("");

            if (auto err = glz::read<opts>(value, serialized); !err)
            {
                return std::nullopt;
            }

            return fmt::format("Expected value to be '{}'", rebind::type_name<T>);
        }

        template <Tuple T, std::size_t I = 0>
        std::optional<std::string> mismatch(T &tuple, const glz::json_t &json)
        {
            static constexpr auto N = std::tuple_size_v<T>;

            if constexpr (I < N)
            {
                using current_t = std::tuple_element_t<I, T>;
                auto err        = mismatch(std::get<I>(tuple), json[I]);

                if (!err.has_value())
                {
                    return mismatch<T, I + 1>(tuple, json);
                }

                return fmt::format("Expected parameter {} to be '{}'", I, rebind::type_name<current_t>);
            }
            else
            {
                return std::nullopt;
            }
        }

        template <typename T>
        std::expected<T, std::string> parse(const std::string &data)
        {
            T rtn{};

            if (auto err = glz::read<opts>(rtn, data); !err)
            {
                return rtn;
            }

            glz::json_t json{};

            if (auto err = glz::read<opts>(json, data); err)
            {
                return std::unexpected{std::string{rebind::utils::find_enum_name(err.ec).value_or("Unknown")}};
            }

            return std::unexpected{mismatch<T>(rtn, json).value_or("<Unknown Error>")};
        }

        template <Tuple T>
            requires(std::tuple_size_v<T> == 0)
        std::expected<T, std::string> parse(const std::string &)
        {
            return {};
        }

        constexpr auto serialize()
        {
            return "";
        }

        template <typename T>
        auto serialize(T &&value)
        {
            static_assert(Serializable<T>, "Given type is not serializable");
            return glz::write<opts>(std::forward<T>(value)).value_or("null");
        }

        template <Arguments T>
        auto serialize(T &&value)
        {
            std::vector<std::string> rtn;
            rtn.reserve(value.size());

            auto unpack = [&]<typename... Ts>(Ts &&...args)
            {
                (rtn.emplace_back(serialize(std::forward<Ts>(args))), ...);
            };
            std::apply(unpack, value.as_tuple());

            return fmt::format("{}", fmt::join(rtn, ", "));
        }
    } // namespace impl

    template <typename Function>
    auto serializer::serialize(Function &&func)
    {
        using resolver  = traits::resolver<Function>;
        using converter = resolver::converter;
        using result    = resolver::result;
        using args      = resolver::args;

        static_assert(impl::Serializable<result> && impl::Serializable<args>,
                      "All arguments as well as the result must be serializable");

        return [resolver = converter::convert(std::forward<Function>(func))](std::unique_ptr<saucer::function_data> data,
                                                                             executor exec)
        {
            const auto &message = *static_cast<function_data *>(data.get());
            const auto params   = impl::parse<args>(message.params.str);

            if (!params)
            {
                return std::invoke(exec.reject, impl::serialize(params.error()));
            }

            auto resolve = [resolve = std::move(exec.resolve)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(resolve, impl::serialize(value...));
            };

            auto reject = [reject = std::move(exec.reject)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(reject, impl::serialize(value...));
            };

            auto exec_param = typename resolver::executor{std::move(resolve), std::move(reject)};
            auto all_params = std::tuple_cat(std::move(params.value()), std::make_tuple(std::move(exec_param)));

            std::apply(resolver, all_params);
        };
    }

    template <typename... Ts>
    auto serializer::serialize_args(Ts &&...params)
    {
        serializer::args rtn;

        rtn.reserve(sizeof...(params), 0);
        (rtn.push_back(impl::serialize(std::forward<Ts>(params))), ...);

        return rtn;
    }

    template <typename T>
    auto serializer::resolve(std::promise<T> promise)
    {
        static_assert(impl::Serializable<T>, "The promise result must be serializable");

        return [promise = std::move(promise)](std::unique_ptr<saucer::result_data> data) mutable
        {
            const auto &res = *static_cast<result_data *>(data.get());

            if constexpr (!std::is_void_v<T>)
            {
                auto parsed = impl::parse<T>(res.result.str);

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
} // namespace saucer::serializers::glaze
