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
            using type  = T;
            using error = E;

          public:
            std::function<void(T)> resolve;
            std::function<void(E)> reject;
        };

        template <typename E>
        struct executor<void, E>
        {
            using type  = void;
            using error = E;

          public:
            std::function<void()> resolve;
            std::function<void(E)> reject;
        };

        template <typename T>
        struct is_executor_impl : std::false_type
        {
        };

        template <typename T, typename E>
        struct is_executor_impl<executor<T, E>> : std::true_type
        {
        };
    } // namespace impl

    enum class launch
    {
        sync,
        manual,
    };

    template <typename T>
    using executor = impl::executor<T, std::string>;

    template <typename T>
    concept is_executor = requires() { requires impl::is_executor_impl<T>::value; };
} // namespace saucer
