#pragma once
#include "json.hpp"
#include <smartview.hpp>

namespace saucer::serializers
{
    json_data::~json_data() = default;
    json_serializer::~json_serializer() = default;

    std::string json_serializer::java_script_serializer() const
    {
        return "JSON.stringify";
    }

    std::shared_ptr<message_data> json_serializer::parse(const std::string &data)
    {
        auto parsed = nlohmann::json::parse(data, nullptr, false);
        if (!parsed.is_discarded())
        {
            if (parsed["id"].is_number_integer() && parsed["params"].is_array() && parsed["name"].is_string())
            {
                auto rtn = std::make_shared<json_data>();
                rtn->id = parsed["id"];
                rtn->data = parsed["params"];
                rtn->function = parsed["name"];

                return rtn;
            }
        }

        return nullptr;
    }

    namespace internal
    {
        template <typename type_t> class is_serializable
        {
            static auto test(...) -> std::uint8_t;
            static auto test(void *) -> std::uint16_t;
            template <typename O> static auto test(type_t *) -> decltype(std::is_constructible_v<nlohmann::json>(std::declval<O>()), std::uint16_t{});

          public:
            static const bool value = sizeof(test(reinterpret_cast<type_t *>(0))) == sizeof(std::uint16_t);
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
        template <typename _rtn_t, typename... _args> struct function_traits<_rtn_t(_args...)>
        {
            using rtn_t = _rtn_t;
            using args_t = std::tuple<remove_const_ref_t<_args>...>;
            static const bool serializable = is_serializable<rtn_t>::value && std::conjunction_v<is_serializable<remove_const_ref_t<_args>>...>;
        };
        template <typename _rtn_t, typename... _args> struct function_traits<_rtn_t(_args...) const>
        {
            using rtn_t = _rtn_t;
            using args_t = std::tuple<remove_const_ref_t<_args>...>;
            static const bool serializable = is_serializable<rtn_t>::value && std::conjunction_v<is_serializable<remove_const_ref_t<_args>>...>;
        };
        template <typename _rtn_t, typename... _args> struct function_traits<_rtn_t(_args...) noexcept>
        {
            using rtn_t = _rtn_t;
            using args_t = std::tuple<remove_const_ref_t<_args>...>;
            static const bool serializable = is_serializable<rtn_t>::value && std::conjunction_v<is_serializable<remove_const_ref_t<_args>>...>;
        };
        template <typename _class, typename _rtn_t, typename... _args> struct function_traits<_rtn_t (_class::*)(_args...) const>
        {
            using rtn_t = _rtn_t;
            using args_t = std::tuple<remove_const_ref_t<_args>...>;
            static const bool serializable = is_serializable<rtn_t>::value && std::conjunction_v<is_serializable<remove_const_ref_t<_args>>...>;
        };

        template <std::size_t I = 0, typename... args, typename callback_t> void tuple_visit(std::tuple<args...> &tuple, const callback_t &callback)
        {
            if constexpr (sizeof...(args) > 0)
            {
                callback(I, std::get<I>(tuple));

                if constexpr (sizeof...(args) > (I + 1))
                {
                    tuple_visit<I + 1>(tuple, callback);
                }
            }
        }
    } // namespace internal

    template <typename func_t> auto json_serializer::encode_function(const func_t &func)
    {
        return [func](const std::shared_ptr<message_data> &data) -> tl::expected<std::string, error> {
            if (auto json_message = std::dynamic_pointer_cast<json_data>(data); json_message)
            {
                const auto &params = json_message->data;

                using traits = internal::function_traits<func_t>;
                using args_t = typename traits::args_t;
                using rtn_t = typename traits::rtn_t;
                static_assert(traits::serializable);

                if (params.size() == std::tuple_size_v<args_t>)
                {
                    args_t args;
                    try
                    {
                        internal::tuple_visit(args, [&params](const std::size_t &I, auto &arg) { arg = params.at(I); });
                    }
                    catch (const nlohmann::json::type_error &)
                    {
                        return tl::make_unexpected(error::argument_mismatch);
                    }

                    nlohmann::json rtn;
                    auto do_call = [func, &rtn](auto &&...args) {
                        if constexpr (std::is_same_v<rtn_t, void>)
                        {
                            func(std::forward<decltype(args)>(args)...);
                        }
                        else
                        {
                            rtn = func(std::forward<decltype(args)>(args)...);
                        }
                    };

                    std::apply(do_call, args);
                    return "JSON.parse(" + nlohmann::json(nlohmann::json(rtn).dump()).dump() + ")"; // ? We dump twice to properly escape
                }
            }
            return tl::make_unexpected(error::parser_mismatch);
        };
    }
} // namespace saucer::serializers