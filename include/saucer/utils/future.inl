#pragma once

#include "future.hpp"

#include <functional>

namespace saucer
{
    template <typename... T>
    auto all(std::future<T> &...futures)
    {
        return all(std::move(futures)...);
    }

    template <typename... T>
    auto all(std::future<T>... futures)
    {
        auto make_tuple = []<typename F>(std::future<F> future)
        {
            if constexpr (!std::same_as<F, void>)
            {
                return std::make_tuple(future.get());
            }
            else
            {
                future.get();
                return std::tuple<>();
            }
        };

        return std::tuple_cat(make_tuple(std::move(futures))...);
    }

    template <typename T, typename Callback>
    void then(std::future<T> future, Callback callback)
    {
        auto fut = std::make_shared<std::future<void>>();

        auto fn = [fut, future = std::move(future), callback = std::move(callback)]() mutable
        {
            std::invoke(callback, future.get());
        };

        *fut = std::async(std::launch::async, std::move(fn));
    }

    template <typename Callback>
    class then_pipe
    {
        Callback m_callback;

      public:
        then_pipe(Callback callback) : m_callback(std::move(callback)) {}

      public:
        template <typename T>
        friend void operator|(std::future<T> &&future, then_pipe pipe)
        {
            then(std::move(future), std::move(pipe.m_callback));
        }
    };

    template <typename Callback>
    then_pipe<Callback> then(Callback callback)
    {
        return then_pipe{std::move(callback)};
    }

    template <typename T>
    void forget(std::future<T> future)
    {
        auto fut = std::make_shared<std::future<void>>();
        *fut     = std::async(std::launch::async, [fut, future = std::move(future)]() mutable { future.get(); });
    }

    struct forget_pipe
    {
        template <typename T>
        friend void operator|(std::future<T> future, [[maybe_unused]] forget_pipe)
        {
            forget(std::move(future));
        }
    };

    inline forget_pipe forget()
    {
        return forget_pipe{};
    }
} // namespace saucer
