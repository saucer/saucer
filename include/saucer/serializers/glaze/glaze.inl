#pragma once

#include "glaze.hpp"

#include <fmt/xchar.h>
#include <fmt/format.h>

#include <rebind/name.hpp>
#include <boost/callable_traits.hpp>

namespace saucer::serializers::glaze
{
    namespace impl
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true};

        template <typename... Ts>
        consteval auto decay_tuple_impl(const std::tuple<Ts...> &) -> std::tuple<std::decay_t<Ts>...>;

        template <typename T>
        using decay_tuple_t = decltype(decay_tuple_impl(std::declval<T>()));

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

        template <typename T>
            requires saucer::is_arguments<T>
        struct serializable_impl<T> : std::true_type
        {
        };

        template <typename T>
        inline constexpr bool serializable = serializable_impl<T>::value;

        template <typename T, std::size_t I = 0>
        error mismatch(T &tuple, const glz::json_t &json)
        {
            static constexpr auto N = std::tuple_size_v<T>;

            if constexpr (I < N)
            {
                using current_t = std::tuple_element_t<I, T>;
                auto serialized = glz::write<opts>(json[I]).value_or("");

                if (auto err = glz::read<opts>(std::get<I>(tuple), serialized); !err)
                {
                    return mismatch<T, I + 1>(tuple, json);
                }

                return {
                    error_code::type_mismatch,
                    fmt::format("Expected parameter {} to be '{}'", I, rebind::type_name<current_t>),
                };
            }
            else
            {
                return {error_code::unknown};
            }
        }

        template <typename T>
        auto serialize(const T &value)
        {
            static_assert(serializable<T>, "Given type is not serializable");

            auto json    = glz::write<opts>(value).value_or("null");
            auto escaped = glz::write<opts>(json).value_or("null");

            return fmt::format("JSON.parse({})", escaped);
        }

        template <typename T>
            requires is_arguments<T>
        auto serialize(const T &value)
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

        template <typename T>
            requires(std::tuple_size_v<T> != 0)
        tl::expected<T, error> parse(const std::string &data)
        {
            T rtn{};

            if (auto err = glz::read<opts>(rtn, data); !err)
            {
                return rtn;
            }

            glz::json_t json{};

            if (auto err = glz::read<opts>(json, data); err)
            {
                auto name = std::string{glz::nameof(err.ec)};
                return tl::make_unexpected(error{error_code::unknown, name});
            }

            return tl::make_unexpected(mismatch<T>(rtn, json));
        }

        template <typename T>
            requires(std::tuple_size_v<T> == 0)
        tl::expected<T, error> parse(const std::string &)
        {
            return {};
        }
    } // namespace impl

    template <typename Function>
    auto serializer::serialize(const Function &func)
    {
        using return_t = boost::callable_traits::return_type_t<Function>;
        using tuple_t  = boost::callable_traits::args_t<Function>;
        using args_t   = impl::decay_tuple_t<tuple_t>;

        static_assert(impl::serializable<return_t> && impl::serializable<args_t>,
                      "All arguments as well as the return type must be serializable");

        return [func](saucer::function_data &data) -> function::result_type
        {
            const auto &message = static_cast<function_data &>(data);
            const auto params   = impl::parse<args_t>(message.params.str);

            if (!params)
            {
                return tl::make_unexpected(params.error());
            }

            std::string result{"null"};

            if constexpr (!std::is_void_v<return_t>)
            {
                result = impl::serialize(std::apply(func, params.value()));
            }
            else
            {
                std::apply(func, params.value());
            }

            return result;
        };
    }

    template <typename... Ts>
    auto serializer::serialize_args(const Ts &...params)
    {
        serializer::args rtn;

        rtn.reserve(sizeof...(params), 0);
        (rtn.push_back(impl::serialize(params)), ...);

        return rtn;
    }

    template <typename T>
    auto serializer::resolve(std::shared_ptr<std::promise<T>> promise)
    {
        static_assert(impl::serializable<T>, "The promise result must be serializable");

        return [promise](saucer::result_data &data) mutable
        {
            const auto &result = static_cast<result_data &>(data);

            if constexpr (!std::is_void_v<T>)
            {
                T value{};

                if (auto error = glz::read<impl::opts>(value, result.result.str); error)
                {
                    auto exception = std::runtime_error{std::string{glz::nameof(error.ec)}};
                    promise->set_exception(std::make_exception_ptr(exception));
                    return;
                }

                promise->set_value(value);
            }
            else
            {
                promise->set_value();
            }
        };
    }
} // namespace saucer::serializers::glaze
