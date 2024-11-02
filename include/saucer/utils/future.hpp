#pragma once

#include <future>

namespace saucer
{
    template <typename... T>
    auto all(std::future<T> &...);

    template <typename... T>
    auto all(std::future<T>...);

    template <typename T, typename Callback, typename>
    void then(std::future<T>, Callback &&);

    template <typename Callback>
    class then_pipe;

    template <typename Callback>
    then_pipe<Callback> then(Callback);
} // namespace saucer

#include "future.inl"
