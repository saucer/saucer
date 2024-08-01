#pragma once

#include <utility>

namespace bindings
{
    template <typename Handle, typename T>
    struct handle
    {
        using type = T;

      public:
        T data;

      public:
        T &value()
        {
            return data;
        }

      public:
        template <typename... Ts>
        static Handle *make(Ts &&...params)
        {
            return new Handle{{std::forward<Ts>(params)...}};
        }

        static Handle *from(T &&data)
        {
            return new Handle{{std::move(data)}};
        }
    };
} // namespace bindings
