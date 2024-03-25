#pragma once

#include "common.h"
#include <saucer/smartview.hpp>

struct cast_callback
{
    void *callback;
    saucer_handle *handle;

  public:
    template <typename R, typename... T>
    operator std::function<R(T...)>()
    {
        return [this](T... params)
        {
            auto cb = reinterpret_cast<R (*)(saucer_handle *, T...)>(callback);
            return cb(handle, std::forward<T>(params)...);
        };
    }
};

template <typename T>
saucer::smartview_core &cast(T *handle)
{
    return *reinterpret_cast<saucer::smartview_core *>(handle);
}
