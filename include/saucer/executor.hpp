#pragma once

#include <string>
#include <functional>

namespace saucer
{
    namespace impl
    {
        template <typename T, typename E>
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
    } // namespace impl

    template <typename T>
    using executor = impl::executor<T, std::string>;
} // namespace saucer
