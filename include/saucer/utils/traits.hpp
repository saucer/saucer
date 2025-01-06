#pragma once

#include "tuple.hpp"
#include "../executor.hpp"

#include <utility>
#include <type_traits>

#include <tuple>
#include <expected>

#include <boost/callable_traits.hpp>

namespace saucer::traits
{
    namespace impl
    {
        template <typename T, typename... Ts>
        consteval auto apply_result(const std::tuple<Ts...> &) -> std::invoke_result_t<T, Ts...>;

        template <typename T, typename... Ts>
        consteval auto can_apply(const std::tuple<Ts...> &) -> std::bool_constant<std::invocable<T, Ts...>>;

        template <typename T, typename D = std::decay_t<T>>
        using arg_transformer_t = std::conditional_t<std::same_as<D, std::string_view>, std::string, D>;
    } // namespace impl

    template <typename T, typename Args>
    using apply_result_t = decltype(impl::apply_result<T>(std::declval<Args>()));

    template <typename T, typename Args>
    static constexpr auto can_apply_v = decltype(impl::can_apply<T>(std::declval<Args>()))::value;

    template <typename T, template <typename...> typename Transform = impl::arg_transformer_t>
    using args_t = tuple::transform_t<boost::callable_traits::args_t<T>, Transform>;

    template <typename T>
    using result_t = boost::callable_traits::return_type_t<T>;

    template <typename T,                                                                                                //
              typename Executor,                                                                                         //
              typename Args,                                                                                             //
              bool TakesExecutor = can_apply_v<T, tuple::add_t<Args, Executor>>,                                         //
              typename Result = apply_result_t<T, std::conditional_t<TakesExecutor, tuple::add_t<Args, Executor>, Args>> //
              >
    struct converter;

    template <typename T, typename Executor, typename Args>
    struct converter<T, Executor, Args, true, void>
    {
        static decltype(auto) convert(T &&callable)
        {
            return std::forward<T>(callable);
        }
    };

    template <typename T, typename R, typename E, typename... Ts, typename Result>
    struct converter<T, executor<R, E>, std::tuple<Ts...>, false, Result>
    {
        static decltype(auto) convert(T &&callable)
            requires std::same_as<Result, R>
        {
            return [callable = std::forward<T>(callable)](Ts &&...args, auto &&executor) mutable
            {
                if constexpr (std::is_void_v<Result>)
                {
                    std::invoke(callable, std::forward<Ts>(args)...);
                    std::invoke(executor.resolve);
                }
                else
                {
                    std::invoke(executor.resolve, std::invoke(callable, std::forward<Ts>(args)...));
                }
            };
        }
    };

    template <typename T, typename... Ts, typename R, typename E>
    struct converter<T, executor<R, E>, std::tuple<Ts...>, false, std::expected<R, E>>
    {
        static decltype(auto) convert(T &&callable)
        {
            return [callable = std::forward<T>(callable)](Ts &&...args, auto &&executor) mutable
            {
                std::invoke(callable, std::forward<Ts>(args)...)
                    .transform(executor.resolve)
                    .transform_error([&executor]<typename... Us>(Us &&...args)
                                     { return std::invoke(executor.reject, std::forward<Us>(args)...), 0; });
            };
        }
    };

    template <typename T, typename Result = result_t<T>, typename Last = tuple::last_t<args_t<T>>>
    struct resolver
    {
        using args     = args_t<T>;
        using error    = void;
        using result   = Result;
        using executor = saucer::executor<Result, void>;

      public:
        using converter = traits::converter<T, executor, args>;
    };

    template <typename T, typename Result, typename R, typename E>
    struct resolver<T, Result, executor<R, E>>
    {
        using args     = tuple::drop_last_t<args_t<T>>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, executor, args>;
    };

    template <typename T, typename R, typename E, typename Last>
    struct resolver<T, std::expected<R, E>, Last>
    {
        using args     = args_t<T>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, executor, args>;
    };
} // namespace saucer::traits
