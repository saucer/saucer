#pragma once
#include "bridge.hpp"
#include <chrono>
#include <functional>

namespace saucer
{
    namespace bridge
    {
        namespace internal
        {
            template <typename type_t> class is_serializable
            {
                static auto test(...) -> std::uint8_t;
                static auto test(void *) -> std::uint16_t;
                template <typename O>
                static auto test(type_t *) -> decltype(std::is_constructible_v<nlohmann::json>(std::declval<O>()), std::uint16_t{});

              public:
                static const bool value = sizeof(test(reinterpret_cast<type_t *>(0))) == sizeof(std::uint16_t);
            };

            template <typename T> struct function_traits : public function_traits<decltype(&T::operator())>
            {
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

        template <typename func_t> void json::expose(const std::string &name, const func_t &function, bool async)
        {
            auto locked = m_callbacks.write();

            // clang-format off
            locked->emplace(name, std::make_pair<callback_t, bool>(
            [this, function](int id, const nlohmann::json &data) {
                using traits = internal::function_traits<func_t>;

                using rtn_t = typename traits::rtn_t;
                using args_t = typename traits::args_t;
                static_assert(traits::serializable, "All types used in the function definition must be serializable");

                if (data.size() == std::tuple_size_v<args_t>)
                {
                    nlohmann::json rtn;
                    auto unpacked_call = [&](auto... args) {
                        if constexpr (!std::is_same_v<void, rtn_t>)
                        {
                            rtn = function(args...);
                        }
                        else
                        {
                            function(args...);
                        }
                    };

                    args_t args;
                    internal::tuple_visit(args, [&](std::size_t index, auto &arg) { arg = data.at(index); });

                    std::apply(unpacked_call, args);
                    resolve(id, rtn);
                }
                else
                {
                    reject(id, "Invalid arguments");
                }
            },
                std::forward<bool>(async)));
            // clang-format on
        }

        template <typename func_t> void json::expose_async(const std::string &name, const func_t &function)
        {
            expose(name, function, true);
        }

        template <typename rtn_t, typename... args_t>
        std::shared_ptr<promise<rtn_t>> json::call(const std::string &function_name, const args_t &...args)
        {
            static_assert(std::conjunction_v<internal::is_serializable<args_t>...>);
            auto locked = m_promises.write();

            // ? We dump twice to escape everything properly.
            const auto idc = m_idc++;
            std::string query("window.saucer._resolve(" + std::to_string(idc) + ", " + function_name + "(");
            (query.append("JSON.parse(" + nlohmann::json(nlohmann::json(args).dump()).dump() + "),"), ...);

            if constexpr (sizeof...(args) > 0)
                query.pop_back();

            query.append("))");

            auto rtn = std::make_shared<promise<rtn_t>>();
            locked->emplace(idc, rtn);

            run_java_script(query);
            return rtn;
        }
    } // namespace bridge
} // namespace saucer