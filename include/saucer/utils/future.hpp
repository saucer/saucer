#pragma once

#include "traits.hpp"

#include <future>
#include <functional>

namespace saucer
{
    template <typename T>
    class packaged_task;

    template <typename R, typename... Ts>
    class packaged_task<R(Ts...)>
    {
        using callback_t = std::move_only_function<R(Ts...)>;

      private:
        callback_t m_callback;
        std::promise<R> m_promise;

      public:
        template <typename... Us>
        packaged_task(Us &&...args) : m_callback(std::forward<Us>(args)...)
        {
        }

      public:
        std::future<R> get_future()
        {
            return m_promise.get_future();
        }

      public:
        template <typename... Us>
            requires std::invocable<callback_t, Us...>
        void operator()(Us &&...args)
        {
            if constexpr (std::is_void_v<R>)
            {
                std::invoke(m_callback, std::forward<Us>(args)...);
                m_promise.set_value();
            }
            else
            {
                m_promise.set_value(std::invoke(m_callback, std::forward<Us>(args)...));
            }
        }
    };

    template <typename T>
    packaged_task(T) -> packaged_task<traits::signature_t<T>>;
} // namespace saucer
