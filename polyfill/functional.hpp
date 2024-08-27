#pragma once

#include <function2/function2.hpp>

// NOLINTBEGIN(*-dcl58-cpp)
namespace std
{
    template <typename T>
    struct move_only_function;

    template <typename R, typename... Ts>
    struct move_only_function<R(Ts...)> : fu2::unique_function<R(Ts...)>
    {
        using fu2::unique_function<R(Ts...)>::unique_function;

      public:
        using result_type = R;
    };
} // namespace std
// NOLINTEND(*-dcl58-cpp)
