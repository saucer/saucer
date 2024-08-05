#pragma once

#include "window.hpp"

#include <poolparty/task.hpp>

namespace saucer
{
    template <typename Callback>
    auto window::dispatch(Callback &&callback)
    {
        auto task = poolparty::packaged_task{std::forward<Callback>(callback)};
        auto rtn  = task.get_future();

        dispatch(callback_t{[task = std::move(task)]() mutable
                            {
                                std::invoke(task);
                            }});

        return rtn;
    }
} // namespace saucer
