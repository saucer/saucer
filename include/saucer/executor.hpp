#pragma once

#include <string>
#include <functional>

namespace saucer
{
    namespace impl
    {
        template <typename Result, typename T>
        consteval auto func_with_arg()
        {
            if constexpr (std::is_void_v<T>)
            {
                return std::type_identity<Result()>{};
            }
            else
            {
                return std::type_identity<Result(T)>{};
            }
        }

        template <typename Result, typename T>
        using func_with_arg_t = decltype(func_with_arg<Result, T>())::type;
    } // namespace impl

    template <typename T, typename E = std::string>
    struct executor
    {
        using result = T;
        using error  = E;

      public:
        std::function<impl::func_with_arg_t<void, T>> resolve;
        std::function<impl::func_with_arg_t<void, E>> reject;
    };
} // namespace saucer
