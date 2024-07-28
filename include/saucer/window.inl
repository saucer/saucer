#pragma once

#include "window.hpp"

#include <future>

namespace saucer
{
    template <typename Callback>
    auto window::dispatch(Callback &&callback) const
    {
        auto task = std::packaged_task{std::forward<Callback>(callback)};
        auto rtn  = task.get_future();

        dispatch(callback_t{[task = std::move(task)]() mutable
                            {
                                std::invoke(task);
                            }});

        return rtn;
    }
} // namespace saucer
