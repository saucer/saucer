#pragma once

#include "app.hpp"

#include <poolparty/task.hpp>

namespace saucer
{
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
} // namespace saucer
