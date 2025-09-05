#pragma once

#include "invoke.hpp"

#include <utility>
#include <algorithm>
#include <string_view>

#include <rebind/member.hpp>

namespace saucer::utils
{
    namespace detail
    {
        template <typename T>
        struct invoker;

        template <typename T>
        constexpr auto noop();

        template <typename Callback, typename T, typename... Ts>
            requires std::invocable<Callback, Ts...>
        constexpr auto invoke(bool, invoker<T>, Callback &&, Ts &&...);
    } // namespace detail

    template <typename T>
    struct detail::invoker
    {
        T *self;

      public:
        template <typename U, typename... Us>
        constexpr auto operator()(U &&callback, Us &&...args)
        {
            return self->parent->invoke(std::forward<U>(callback), std::forward<Us>(args)...);
        }
    };

    template <typename T>
    constexpr auto detail::noop()
    {
        return T{};
    }

    template <>
    constexpr auto detail::noop<void>()
    {
    }

    template <typename Callback, typename T, typename... Ts>
        requires std::invocable<Callback, Ts...>
    constexpr auto detail::invoke(bool cond, invoker<T> invoker, Callback &&callback, Ts &&...args)
    {
        using result = std::invoke_result_t<Callback, Ts...>;

        if (!cond)
        {
            return noop<result>();
        }

        return invoker(std::forward<Callback>(callback), std::forward<Ts>(args)...);
    }

    template <typename T>
    struct detail::callback
    {
        static constexpr std::size_t N = 128;

      public:
        T value;
        char name[N + 1]{};

      public:
        constexpr callback(T value, std::string_view func = __builtin_FUNCTION()) : value(value)
        {
            auto pure = func.substr(0, func.find_first_of("(<"));
            std::copy_n(pure.data(), std::min(pure.size(), N), name); // NOLINT(*-stringview-data-usage)
        }
    };

    template <typename Callback, typename T, typename... Ts>
        requires std::invocable<Callback, Ts...>
    constexpr auto invoke(Callback &&callback, T *self, Ts &&...args)
    {
        return detail::invoke(static_cast<bool>(self), detail::invoker<T>{self}, std::forward<Callback>(callback),
                              std::forward<Ts>(args)...);
    }

    template <detail::callback Callback, typename T, typename... Ts>
    constexpr auto invoke(T *self, Ts &&...args)
    {
        static constexpr auto name = rebind::member_name<Callback.value>;
        static constexpr auto pure = name.substr(0, name.find_first_of("(<"));
        static_assert(pure == Callback.name, "Name of implementation does not match interface");

        return invoke(Callback.value, self, self, std::forward<Ts>(args)...);
    }
} // namespace saucer::utils
