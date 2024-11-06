#pragma once

#include <string>
#include <functional>

namespace saucer
{
    template <typename T, typename E = std::string>
    struct executor
    {
        std::function<void(T)> resolve;
        std::function<void(E)> reject;
    };

    template <typename E>
    struct executor<void, E>
    {
        std::function<void()> resolve;
        std::function<void(E)> reject;
    };

    template <typename T>
    struct executor<T, void>
    {
        std::function<void(T)> resolve;
        std::function<void()> reject;
    };
} // namespace saucer
