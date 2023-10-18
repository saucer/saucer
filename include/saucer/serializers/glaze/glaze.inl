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
            auto &message = static_cast<glaze_function_data &>(data);

            decayed_t params;
            glz::parse_error error;

            if constexpr (std::tuple_size_v<decayed_t> > 0)
            {
                error = glz::read_json<decayed_t>(params, message.params.str);
            }

            if (error)
            {
                return tl::make_unexpected(serializer_error::type_mismatch);
            }

            std::string result{"null"};

            if constexpr (!std::is_void_v<return_t>)
            {
                result = glz::write_json(std::apply(func, params));
            }
            else
            {
                std::apply(func, params);
            }

            auto escaped = glz::write_json(result);
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
                auto json    = glz::write_json(value);
                auto escaped = glz::write_json(json);

                return fmt::format("JSON.parse({})", escaped);
            };

            if constexpr (is_arguments<T>)
            {
                using tuple_t = typename T::tuple_t;
                auto tuple    = static_cast<tuple_t>(value);

                std::vector<std::string> rtn;
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
            auto &result = static_cast<glaze_result_data &>(data);

            if constexpr (!std::is_void_v<T>)
            {
                auto value = glz::read_json<T>(result.result.str);

                if (!value.has_value())
                {
                    auto error     = static_cast<std::uint32_t>(value.error());
                    auto exception = std::runtime_error{std::to_string(error)};

                    promise->set_exception(std::make_exception_ptr(exception));
                    return;
                }

                promise->set_value(value.value());
            }
            else
            {
                promise->set_value();
            }
        };
    }
} // namespace saucer::serializers
