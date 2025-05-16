#pragma once

#include <functional>
#include <string_view>

namespace saucer
{
    namespace impl
    {
        template <typename R, typename T>
        struct fn_with_arg
        {
            using type = R(T);
        };

        template <typename R>
        struct fn_with_arg<R, void>
        {
            using type = R();
        };

        template <typename R, typename T>
        using fn_with_arg_t = fn_with_arg<R, T>::type;
    } // namespace impl

    template <typename T, typename E = std::string_view>
    struct executor
    {
        std::function<impl::fn_with_arg_t<void, T>> resolve;
        std::function<impl::fn_with_arg_t<void, E>> reject;
    };
} // namespace saucer
