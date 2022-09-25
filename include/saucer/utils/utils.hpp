#pragma once
#include <future>

namespace saucer
{
    template <typename... T> auto all(std::future<T> &...);
    template <typename... T> auto all(std::future<T> &&...);

    template <typename Callback> struct then_pipe;
    template <typename Callback> then_pipe<Callback> then(Callback &&);
    template <typename T, typename Callback, typename> void then(std::future<T> &&, Callback &&);

    struct forget_pipe;
    forget_pipe forget();
    template <typename T> void forget(std::future<T> &&);
} // namespace saucer

#include "utils.inl"