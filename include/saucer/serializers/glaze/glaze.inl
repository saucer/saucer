#pragma once

#include "glaze.hpp"

#include <fmt/args.h>
#include <fmt/xchar.h>
#include <fmt/format.h>

#include <rebind/name.hpp>
#include <boost/callable_traits.hpp>

namespace saucer::serializers::detail::glaze
{
    static constexpr auto opts = glz::opts{.error_on_missing_keys = true, .raw_string = false};

    template <typename... T>
    consteval auto decay(const std::tuple<T...> &) -> std::tuple<std::decay_t<T>...>;

    template <typename T>
    using decay_t = decltype(decay(std::declval<T>()));

    template <typename T>
    struct serializable : std::false_type
    {
    };

    template <>
    struct serializable<void> : std::true_type
    {
    };

    template <>
    struct serializable<std::tuple<>> : std::true_type
    {
    };

    template <typename... T>
    struct serializable<saucer::arguments<T...>> : serializable<typename saucer::arguments<T...>::underlying>
    {
    };

    template <typename T>
        requires glz::read_supported<opts.format, T>
    struct serializable<T> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool serializable_v = serializable<T>::value;

    template <typename T, std::size_t I = 0>
    error mismatch(T &tuple, const glz::json_t &json)
    {
        static constexpr auto N = std::tuple_size_v<T>;

        if constexpr (I >= N)
        {
            return {error_code::unknown};
        }
        else
        {
            using current_t = std::tuple_element_t<I, T>;

            auto serialized = glz::write<opts>(json[I]).value_or("");
            auto err        = glz::read<opts>(std::get<I>(tuple), serialized);

            if (!err)
            {
                return mismatch<T, I + 1>(tuple, json);
            }

            return {
                error_code::type_mismatch,
                fmt::format("Expected parameter {} to be '{}'", I, rebind::type_name<current_t>),
            };
        }
    }
} // namespace saucer::serializers::detail::glaze

namespace saucer::serializers
{
    template <typename Function>
    auto glaze::serialize(const Function &func)
    {
        using return_t  = boost::callable_traits::return_type_t<Function>;
        using args_t    = boost::callable_traits::args_t<Function>;
        using decayed_t = detail::glaze::decay_t<args_t>;
        using detail::glaze::opts;

        static_assert(detail::glaze::serializable_v<return_t> && detail::glaze::serializable_v<decayed_t>,
                      "All arguments as well as the return type must be serializable");

        return [func](function_data &data) -> function::result_type
        {
            const auto &message = static_cast<glaze_function_data &>(data);

            decayed_t params{};

            if constexpr (std::tuple_size_v<decayed_t> > 0)
            {
                if (auto err = glz::read<opts>(params, message.params.str); err)
                {
                    glz::json_t json{};

                    if (err = glz::read<opts>(json, message.params.str); err)
                    {
                        return tl::make_unexpected(error{
                            error_code::unknown,
                            std::string{glz::nameof(err.ec)},
                        });
                    }

                    return tl::make_unexpected(detail::glaze::mismatch<decayed_t>(params, json));
                }
            }

            std::string result{"null"};

            if constexpr (!std::is_void_v<return_t>)
            {
                auto serialized = glz::write<detail::glaze::opts>(std::apply(func, params));

                if (!serialized)
                {
                    return tl::make_unexpected(error{
                        error_code::unknown,
                        std::string{glz::nameof(serialized.error().ec)},
                    });
                }

                result = std::move(serialized.value());
            }
            else
            {
                std::apply(func, params);
            }

            auto escaped = glz::write<detail::glaze::opts>(result);

            if (!escaped)
            {
                return tl::make_unexpected(error{
                    error_code::unknown,
                    std::string{glz::nameof(escaped.error().ec)},
                });
            }

            return fmt::format("JSON.parse({})", escaped.value());
        };
    }

    template <typename... Params>
    auto glaze::serialize_args(const Params &...params)
    {
        fmt::dynamic_format_arg_store<fmt::format_context> rtn;

        const auto unpack = [&]<typename T>(const T &value)
        {
            static_assert(detail::glaze::serializable_v<T>, "All arguments must be serializable");

            const auto serialize = []<typename O>(const O &value)
            {
                auto json    = glz::write<detail::glaze::opts>(value).value_or("null");
                auto escaped = glz::write<detail::glaze::opts>(json).value_or("null");

                return fmt::format("JSON.parse({})", escaped);
            };

            if constexpr (is_arguments<T>)
            {
                using underlying = typename T::underlying;
                auto tuple       = static_cast<underlying>(value);

                std::vector<std::string> rtn{};
                rtn.reserve(std::tuple_size_v<underlying>);

                std::apply([&](const auto &...args) { (rtn.emplace_back(serialize(args)), ...); }, tuple);
                return fmt::format("{}", fmt::join(rtn, ", "));
            }
            else
            {
                return serialize(value);
            }
        };

        (rtn.push_back(unpack(params)), ...);

        return rtn;
    }

    template <typename T>
    auto glaze::resolve(std::shared_ptr<std::promise<T>> promise)
    {
        static_assert(detail::glaze::serializable_v<T>, "The promise result must be serializable");

        return [promise](result_data &data) mutable
        {
            const auto &result = static_cast<glaze_result_data &>(data);

            if constexpr (!std::is_void_v<T>)
            {
                T value{};

                if (auto error = glz::read<detail::glaze::opts>(value, result.result.str); error)
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
} // namespace saucer::serializers
