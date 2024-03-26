#pragma once

#include "color.h"
#include "embed.h"
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
auto *cast(T *handle) = delete;

template <>
inline auto *cast(saucer::webview::embedded_files *handle)
{
    return reinterpret_cast<saucer_embedded_files *>(handle);
}

template <>
inline auto *cast(saucer_embedded_files *handle)
{
    return reinterpret_cast<saucer::webview::embedded_files *>(handle);
}

template <>
inline auto *cast(saucer_message_data *handle)
{
    return reinterpret_cast<saucer::message_data *>(handle);
}


template <>
inline auto *cast(saucer::color *handle)
{
    return reinterpret_cast<saucer_color *>(handle);
}

template <>
inline auto *cast(saucer_color *handle)
{
    return reinterpret_cast<saucer::color *>(handle);
}
