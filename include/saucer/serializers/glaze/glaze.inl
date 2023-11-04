#pragma once

#include "glaze.hpp"

#include <fmt/args.h>
#include <fmt/format.h>

#include <boost/callable_traits.hpp>

namespace saucer::serializers::detail::glaze
{
    template <typename... T>
    constexpr auto decay(const std::tuple<T...> &) -> std::tuple<std::decay_t<T>...>;

    template <typename T>
    using decay_t = decltype(decay(std::declval<T>()));

    template <typename T>
    concept GlzSerializable = requires(T &value) { glz::read_json(value, ""); };

    template <typename T>
    struct serializable : public std::false_type
    {
    };

    template <>
    struct serializable<void> : public std::true_type
    {
    };

    template <typename T>
        requires GlzSerializable<T>
    struct serializable<T> : public std::true_type
    {
    };

    template <typename... T>
        requires(GlzSerializable<T> && ...)
    struct serializable<std::tuple<T...>> : public std::true_type
    {
    };

    template <typename... T>
        requires(GlzSerializable<T> && ...)
    struct serializable<saucer::arguments<T...>> : public std::true_type
    {
    };

    template <class T>
    inline constexpr bool serializable_v = serializable<T>::value;

    static constexpr auto opts = glz::opts{.error_on_missing_keys = true, .raw_string = false};
} // namespace saucer::serializers::detail::glaze

namespace saucer::serializers
{
    template <typename Function>
    auto glaze::serialize(const Function &func)
    {
        using return_t  = boost::callable_traits::return_type_t<Function>;
        using args_t    = boost::callable_traits::args_t<Function>;
        using decayed_t = detail::glaze::decay_t<args_t>;

        static_assert(detail::glaze::serializable_v<return_t> && detail::glaze::serializable_v<decayed_t>,
                      "All arguments as well as the return type must be serializable");

        return [func](function_data &data) -> function::result_type
        {
            const auto &message = static_cast<glaze_function_data &>(data);

            decayed_t params{};

            if constexpr (std::tuple_size_v<decayed_t> > 0)
            {
                const auto error = glz::read<detail::glaze::opts, decayed_t>(params, message.params.str);

                switch (error.ec)
                {
                case glz::error_code::missing_key:
                    return tl::make_unexpected(serializer_error::argument_count_mismatch);
                case glz::error_code::none:
                    break;
                default:
                    return tl::make_unexpected(serializer_error::type_mismatch);
                }
            }

            std::string result{"null"};

            if constexpr (!std::is_void_v<return_t>)
            {
                glz::write<detail::glaze::opts>(std::apply(func, params), result);
            }
            else
            {
                std::apply(func, params);
            }

            std::string escaped(result.size() + 2, '\0');
            glz::write<detail::glaze::opts>(result, escaped);

            return fmt::format("JSON.parse({})", escaped);
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
                std::string json{};
                glz::write<detail::glaze::opts>(value, json);

                std::string escaped(json.size() + 2, '\0');
                glz::write<detail::glaze::opts>(json, escaped);

                return fmt::format("JSON.parse({})", escaped);
            };

            if constexpr (is_arguments<T>)
            {
                using tuple_t = typename T::tuple_t;
                auto tuple    = static_cast<tuple_t>(value);

                std::vector<std::string> rtn{};
                rtn.reserve(std::tuple_size_v<tuple_t>);

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

                const auto error = glz::read<detail::glaze::opts>(value, result.result.str);

                if (error)
                {
                    auto code      = static_cast<std::uint32_t>(error);
                    auto exception = std::runtime_error{std::to_string(code)};

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
