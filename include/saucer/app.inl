#pragma once

#include "app.hpp"

#include <poolparty/task.hpp>

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
                return app->dispatch([this, ptr] { return operator()(ptr); });
            }

            delete ptr;
        }
    };

    template <bool Get, typename Callback>
    auto application::dispatch(Callback &&callback) const
    {
        auto task = poolparty::packaged_task{std::forward<Callback>(callback)};
        auto rtn  = task.get_future();

        post([task = std::move(task)]() mutable { std::invoke(task); });

        if constexpr (Get)
        {
            return rtn.get();
        }
        else
        {
            return rtn;
        }
    }

    template <typename T, typename... Ts>
    safe_ptr<T> application::make(Ts &&...args) const
    {
        if (!thread_safe())
        {
            return dispatch([this, ... args = std::forward<Ts>(args)]() mutable { return make<T>(std::forward<Ts>(args)...); });
        }

        return safe_ptr<T>{new T{std::forward<Ts>(args)...}, safe_delete<T>{.app = this}};
    }
} // namespace saucer
