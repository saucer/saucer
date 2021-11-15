#pragma once
#include "promise.hpp"
#include <atomic>

namespace saucer
{
    inline void base_promise::resolve(const nlohmann::json &) {}

    template <typename type_t> void promise<type_t>::resolve(const nlohmann::json &result)
    {
        m_result = result;

        if constexpr (!std::is_same_v<type_t, void>)
        {
            if (m_callback)
            {
                m_callback(m_result);
            }
        }
    }

    template <typename... args_t> void promise<std::tuple<args_t...>>::resolve(const nlohmann::json &result)
    {
        m_result = result;
    }

    template <typename type_t> void promise<type_t>::then(const callback_t &callback)
    {
        m_callback = callback;
    }

    template <typename... args_t> void promise<std::tuple<args_t...>>::finish(const args_t &...results)
    {
        m_callback(results...);
    }

    template <typename... args_t> void promise<std::tuple<args_t...>>::then(const callback_t &callback)
    {
        m_callback = callback;
    }

    template <typename... args> std::shared_ptr<promise<std::tuple<args...>>> all(std::shared_ptr<promise<args>>... promises)
    {
        auto counter = std::make_shared<std::atomic<std::uint64_t>>();
        auto rtn = std::make_shared<promise<std::tuple<args...>>>();

        auto callback = [counter, rtn, promises...](auto...) {
            counter->fetch_add(1);
            if (counter->load() == sizeof...(promises))
            {
                rtn->finish((promises->m_result)...);
            }
        };

        (promises->then(callback), ...);
        return rtn;
    }
} // namespace saucer