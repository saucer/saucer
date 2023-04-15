#pragma once
#include "future.hpp"

namespace saucer
{
    template <typename... T> auto all(std::future<T> &...futures)
    {
        return all(std::move(futures)...);
    }

    template <typename... T> auto all(std::future<T> &&...futures)
    {
        return std::tuple_cat([](auto &&future) {
            if constexpr (std::is_same_v<decltype(future.get()), void>)
            {
                return std::tuple<>();
            }
            else
            {
                return std::make_tuple(future.get());
            }
        }(std::move(futures))...);
    }

    template <typename T, typename Callback> void then(std::future<T> &&future, Callback &&callback)
    {
        auto fut = std::make_shared<std::future<void>>();

        *fut = std::async(std::launch::async, [future = std::move(future), fut, callback]() mutable {
            callback(future.get()); //
        });
    }

    template <typename Callback> struct then_pipe
    {
      private:
        Callback m_callback;

      public:
        then_pipe(Callback &&callback) : m_callback(std::move(callback)) {}

      public:
        template <typename T> friend void operator|(std::future<T> &&future, then_pipe pipe)
        {
            then(std::move(future), pipe.m_callback);
        }
    };

    template <typename Callback> then_pipe<Callback> then(Callback &&callback)
    {
        return then_pipe{std::forward<Callback>(callback)};
    }

    template <typename T> void forget(std::future<T> &&future)
    {
        auto fut = std::make_shared<std::future<void>>();
        *fut = std::async(std::launch::async, [future = std::move(future), fut]() mutable { future.get(); });
    }

    struct forget_pipe
    {
        template <typename T> friend void operator|(std::future<T> &&future, [[maybe_unused]] forget_pipe)
        {
            forget(std::move(future));
        }
    };

    inline forget_pipe forget()
    {
        return forget_pipe{};
    }
} // namespace saucer