#pragma once

#include "window.hpp"

#include <future>

namespace saucer
{
    template <typename Callback>
    auto window::dispatch(Callback &&callback) const
    {
        // We can't use std::packaged_task because MSVC sucks ass: https://github.com/microsoft/STL/issues/321
        using result_t = std::invoke_result_t<Callback>;

        std::promise<result_t> result;
        auto rtn = result.get_future();

        dispatch(callback_t{[callback = std::forward<Callback>(callback), result = std::move(result)]() mutable
                            {
                                if constexpr (!std::is_void_v<result_t>)
                                {
                                    result.set_value(std::invoke(callback));
                                }
                                else
                                {
                                    std::invoke(callback);
                                    result.set_value();
                                }
                            }});

        return rtn;
    }
} // namespace saucer
