#pragma once
#include "promise.hpp"
#include <atomic>

namespace saucer
{
    inline void base_promise::resolve(const nlohmann::json &) {}

    template <typename type_t> void promise<type_t>::resolve(const nlohmann::json &result)
    {
        m_result = result;

        if (m_callback)
        {
            if constexpr (!std::is_same_v<type_t, void>)
            {
                try
                {
                    m_callback(m_result);
                }
                catch (const nlohmann::json::type_error &)
                {
                    if constexpr (std::is_default_constructible_v<type_t>)
                    {
                        m_callback(type_t{});
                    }
                }
            }
            else
            {
                m_callback();
            }
        }
    }

    template <typename type_t> void promise<type_t>::then(const callback_t &callback)
    {
        m_callback = callback;
    }

    namespace internal
    {
        template <typename type_t> auto tuple_or_data(const nlohmann::json &data)
        {
            if constexpr (std::is_same_v<type_t, void>)
            {
                return std::tuple<>();
            }
            else
            {
                try
                {
                    return std::tuple<type_t>(data);
                }
                catch (const nlohmann::json::type_error &)
                {
                    return std::tuple<type_t>();
                }
            }
        }

        template <typename... args_t> void grouped_promise<args_t...>::then(const callback_t &callback)
        {
            m_callback = callback;
        }

        template <typename... args_t> void grouped_promise<args_t...>::resolve(args_t... args)
        {
            if (m_callback)
            {
                m_callback(args...);
            }
        }
    } // namespace internal

    template <typename... args> auto all(std::shared_ptr<promise<args>>... promises)
    {
        auto counter = std::make_shared<std::atomic<std::uint64_t>>();
        auto rtn = std::make_shared<internal::grouped_promise<decltype(std::tuple_cat(
            std::tuple<>(), std::conditional_t<std::is_same_v<typename decltype(promises)::element_type::type, void>, std::tuple<>,
                                               std::tuple<typename decltype(promises)::element_type::type>>()...))>>();

        auto then_callback = [rtn, counter, promises...](const auto &...) {
            counter->fetch_add(1);

            if (counter->load() == sizeof...(promises))
            {
                auto result =
                    std::tuple_cat(std::tuple<>(), internal::tuple_or_data<typename decltype(promises)::element_type::type>(promises->m_result)...);
                auto call_fn = [&](auto... params) { rtn->resolve(params...); };
                std::apply(call_fn, result);
            }
        };

        (promises->then(then_callback), ...);
        return rtn;
    }
} // namespace saucer