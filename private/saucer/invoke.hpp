#pragma once

#include <utility>
#include <concepts>

#include <algorithm>
#include <string_view>

#include <rebind/member.hpp>

namespace saucer
{
    template <typename T>
    concept HasParent = requires(T *value) {
        { value->parent };
    };

    template <typename T>
    struct invoker
    {
        T *self;

      public:
        template <typename U, typename... Us>
        constexpr auto operator()(U &&callback, Us &&...args)
        {
            return self->invoke(std::forward<U>(callback), std::forward<Us>(args)...);
        }
    };

    template <HasParent T>
    struct invoker<T>
    {
        T *self;

      public:
        template <typename U, typename... Us>
        constexpr auto operator()(U &&callback, Us &&...args)
        {
            return self->parent->invoke(std::forward<U>(callback), std::forward<Us>(args)...);
        }
    };

    template <typename Invoker, typename Callback, typename... Ts>
        requires std::invocable<Callback, Ts...>
    constexpr auto invoke(bool cond, Invoker invoker, Callback &&callback, Ts &&...args)
    {
        using result = std::invoke_result_t<Callback, Ts...>;

        if (cond)
        {
            return invoker(std::forward<Callback>(callback), std::forward<Ts>(args)...);
        }

        if constexpr (std::is_void_v<result>)
        {
            return;
        }
        else
        {
            return result{};
        }
    }

    template <typename T, typename Callback, typename... Ts>
        requires std::invocable<Callback, Ts...>
    constexpr auto invoke(T *self, Callback &&callback, Ts &&...args)
    {
        return invoke(static_cast<bool>(self), invoker<T>{self}, std::forward<Callback>(callback), std::forward<Ts>(args)...);
    }

    template <auto Callback, typename T, typename... Ts>
        requires std::invocable<decltype(Callback), T *, Ts...>
    constexpr auto invoke_impl(T *self, Ts &&...args)
    {
        return invoke(static_cast<bool>(self), invoker<T>{self}, Callback, self, std::forward<Ts>(args)...);
    }

    template <auto Callback, typename T, typename U, typename... Ts>
        requires std::invocable<decltype(Callback), U *, Ts...>
    constexpr auto invoke_impl(T *self, U *impl, Ts &&...args)
    {
        return invoke(static_cast<bool>(impl), invoker<T>{self}, Callback, impl, std::forward<Ts>(args)...);
    }

    template <typename T>
    struct callback
    {
        static constexpr std::size_t N = 128;

      public:
        T value;
        char name[N + 1]{};

      public:
        constexpr callback(T value, std::string_view func = __builtin_FUNCTION()) : value(value)
        {
            auto pure = func.substr(0, func.find_first_of("(<"));
            std::copy_n(pure.data(), std::min(N, pure.size()), name); // NOLINT(*-stringview-data-usage)
        }
    };

    template <callback Callback, typename... Ts>
    constexpr auto invoke(Ts &&...args)
    {
        static_assert(rebind::member_name<Callback.value>.ends_with(Callback.name), "Name of implementation does not match interface");
        return invoke_impl<Callback.value>(std::forward<Ts>(args)...);
    }
} // namespace saucer
