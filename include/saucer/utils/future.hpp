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

    template <typename T>
    [[deprecated("When discarding smartview::evaluate results, consider using smartview::execute instead.")]]
    void forget(std::future<T>);

    struct forget_pipe;

    forget_pipe forget();
} // namespace saucer

#include "future.inl"
