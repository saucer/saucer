#pragma once

#include "app.hpp"

namespace saucer
{
    template <typename T>
    struct safe_delete
    {
        const application *app;

      public:
        void operator()(T *ptr) const
        {
            if (!app->thread_safe())
            {
                return app->invoke([this, ptr] { return operator()(ptr); });
            }

            delete ptr;
        }
    };

    template <typename Callback, typename... Ts>
    auto application::invoke(Callback &&callback, Ts &&...args) const
    {
        if (thread_safe())
        {
            return std::invoke(std::forward<Callback>(callback), std::forward<Ts>(args)...);
        }

        auto task_callback = [callback = std::forward<Callback>(callback), ... args = std::forward<Ts>(args)]() mutable
        {
            return std::invoke(std::forward<Callback>(callback), std::forward<Ts>(args)...);
        };

        auto task   = std::packaged_task{task_callback};
        auto future = task.get_future();

        post([task = std::move(task)]() mutable { std::invoke(task); });

        return future.get();
    }

    template <typename T, typename... Ts>
    safe_ptr<T> application::make(Ts &&...args) const
    {
        if (!thread_safe())
        {
            return invoke([this, ... args = std::forward<Ts>(args)]() mutable { return make<T>(std::forward<Ts>(args)...); });
        }

        return safe_ptr<T>{new T{std::forward<Ts>(args)...}, safe_delete<T>{.app = this}};
    }
} // namespace saucer
