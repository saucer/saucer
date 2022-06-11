#pragma once
#include "json.hpp"
#include "../smartview.hpp"

namespace saucer::serializers
{
    inline json::~json() = default;
    inline json_result_data::~json_result_data() = default;
    inline json_function_data::~json_function_data() = default;

    inline std::string json::java_script_serializer() const
    {
        return "JSON.stringify";
    }

    inline std::string json::initialization_script() const
    {
        return "";
    }

    inline std::shared_ptr<message_data> json::parse(const std::string &data) const
    {
        auto parsed = nlohmann::json::parse(data, nullptr, false);
        if (!parsed.is_discarded())
        {
            if (parsed["id"].is_number_integer())
            {
                if (parsed["params"].is_array() && parsed["name"].is_string())
                {
                    auto rtn = std::make_shared<json_function_data>();
                    rtn->id = parsed["id"];
                    rtn->data = parsed["params"];
                    rtn->function = parsed["name"];

                    return rtn;
                }

                if (!parsed["result"].is_discarded())
                {
                    auto rtn = std::make_shared<json_result_data>();
                    rtn->id = parsed["id"];
                    rtn->data = parsed["result"];

                    return rtn;
                }
            }
        }

        return nullptr;
    }

    namespace internal
    {
        template <typename Type> class is_serializable
        {
            static auto test(...) -> std::uint8_t;
            static auto test(void *) -> std::uint16_t;
            template <typename O> static auto test(Type *) -> decltype(std::is_constructible_v<nlohmann::json>(std::declval<O>()), std::uint16_t{});

          public:
            static const bool value = sizeof(test(reinterpret_cast<Type *>(0))) == sizeof(std::uint16_t);
        };

        template <typename T> struct remove_const_ref
        {
            using type = T;
        };
        template <typename T> struct remove_const_ref<const T &>
        {
            using type = T;
        };
        template <typename T> using remove_const_ref_t = typename remove_const_ref<T>::type;

        template <typename T> struct function_traits : public function_traits<decltype(&T::operator())>
        {
        };
        template <typename Return, typename... Args> struct function_traits<Return(Args...)>
        {
            using rtn_t = Return;
            using args_t = std::tuple<remove_const_ref_t<Args>...>;
            static const bool serializable = is_serializable<Return>::value && std::conjunction_v<is_serializable<remove_const_ref_t<Args>>...>;
        };
        template <typename Return, typename... Args> struct function_traits<Return(Args...) const>
        {
            using rtn_t = Return;
            using args_t = std::tuple<remove_const_ref_t<Args>...>;
            static const bool serializable = is_serializable<Return>::value && std::conjunction_v<is_serializable<remove_const_ref_t<Args>>...>;
        };
        template <typename Return, typename... Args> struct function_traits<Return(Args...) noexcept>
        {
            using rtn_t = Return;
            using args_t = std::tuple<remove_const_ref_t<Args>...>;
            static const bool serializable = is_serializable<Return>::value && std::conjunction_v<is_serializable<remove_const_ref_t<Args>>...>;
        };
        template <typename _class, typename Return, typename... Args> struct function_traits<Return (_class::*)(Args...) const>
        {
            using rtn_t = Return;
            using args_t = std::tuple<remove_const_ref_t<Args>...>;
            static const bool serializable = is_serializable<rtn_t>::value && std::conjunction_v<is_serializable<remove_const_ref_t<Args>>...>;
        };

        template <std::size_t I = 0, typename... Args, typename Callback> void tuple_visit(std::tuple<Args...> &tuple, const Callback &callback)
        {
            if constexpr (sizeof...(Args) > 0)
            {
                callback(I, std::get<I>(tuple));

                if constexpr (sizeof...(Args) > (I + 1))
                {
                    tuple_visit<I + 1>(tuple, callback);
                }
            }
        }
        template <std::size_t I = 0, typename... Args, typename Callback> void tuple_visit(const std::tuple<Args...> &tuple, const Callback &callback)
        {
            if constexpr (sizeof...(Args) > 0)
            {
                callback(I, std::get<I>(tuple));

                if constexpr (sizeof...(Args) > (I + 1))
                {
                    tuple_visit<I + 1>(tuple, callback);
                }
            }
        }

        template <typename> struct is_args : std::false_type
        {
        };
        template <typename... T> struct is_args<arguments<T...>> : std::true_type
        {
        };
        template <typename T> inline constexpr bool is_args_v = is_args<T>::value;

        template <typename T> struct arg_to_tuple
        {
            using type = std::tuple<>;
        };
        template <typename... T> struct arg_to_tuple<arguments<T...>>
        {
            using type = std::tuple<T...>;
        };
        template <typename T> using arg_to_tuple_v = typename arg_to_tuple<T>::type;
    } // namespace internal

    template <typename Function> auto json::serialize_function(const Function &func)
    {
        return [func](const std::shared_ptr<function_data> &data) -> tl::expected<std::string, error> {
            auto json_message = std::dynamic_pointer_cast<json_function_data>(data);
            using traits = internal::function_traits<Function>;
            using args_t = typename traits::args_t;
            using rtn_t = typename traits::rtn_t;
            static_assert(traits::serializable);

            const auto &params = json_message->data;
            if (params.size() == std::tuple_size_v<args_t>)
            {
                args_t args;
                try
                {
                    internal::tuple_visit(args, [&params](const std::size_t &I, auto &arg) { arg = static_cast<std::decay_t<decltype(arg)>>(params.at(I)); });
                }
                catch (const nlohmann::json::type_error &)
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

                return "JSON.parse(" + nlohmann::json(nlohmann::json(rtn).dump()).dump() + ")"; // ? We dump twice to properly escape
            }

            return tl::make_unexpected(error::argument_count_mismatch);
        };
    }

    template <typename... Params> auto json::serialize_arguments(const Params &...params)
    {
        fmt::dynamic_format_arg_store<fmt::format_context> args;

        auto unpack_args = [&args](const auto &arg) {
            using arg_t = decltype(arg);
            std::string rtn;

            if constexpr (internal::is_args_v<std::decay_t<arg_t>>)
            {
                using tuple_t = internal::arg_to_tuple_v<std::decay_t<arg_t>>;

                internal::tuple_visit(arg, [&rtn](const std::size_t &I, const auto &item) {
                    rtn += "JSON.parse(" + nlohmann::json(nlohmann::json(item).dump()).dump() + ")"; // NOLINT
                    if ((I + 1) < std::tuple_size_v<tuple_t>)
                    {
                        rtn += ",";
                    }
                });
            }
            else
            {
                rtn = "JSON.parse(" + nlohmann::json(nlohmann::json(arg).dump()).dump() + ")"; // NOLINT
            }

            args.push_back(rtn);
        };

        (unpack_args(params), ...);
        return args;
    }

    template <typename T> auto json::resolve_promise(const std::shared_ptr<promise<T>> &promise)
    {
        return [promise](const std::shared_ptr<result_data> &data) -> tl::expected<void, serializer::error> {
            auto json_data = std::dynamic_pointer_cast<json_result_data>(data);
            if constexpr (std::is_same_v<T, void>)
            {
                promise->resolve();
                return {};
            }
            else
            {
                try
                {
                    promise->resolve(json_data->data);
                    return {};
                }
                catch (const nlohmann::json::type_error &)
                {
                    return tl::make_unexpected(error::type_mismatch);
                }
            }
        };
    }
} // namespace saucer::serializers