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
        template <typename T, typename Args>
        struct can_apply : std::false_type
        {
        };

        template <typename T, typename... Ts>
            requires std::invocable<T, Ts...>
        struct can_apply<T, std::tuple<Ts...>> : std::true_type
        {
        };

        template <typename T, typename Args>
        struct apply_result;

        template <typename T, typename... Ts>
            requires std::invocable<T, Ts...>
        struct apply_result<T, std::tuple<Ts...>>
        {
            using type = std::invoke_result_t<T, Ts...>;
        };

        template <typename T>
        struct has_reference : std::false_type
        {
        };

        template <typename... Ts>
            requires((std::is_lvalue_reference_v<Ts> && !std::is_const_v<std::remove_reference_t<Ts>>) || ...)
        struct has_reference<std::tuple<Ts...>> : std::true_type
        {
        };

        template <typename T, typename D = std::decay_t<T>>
        using arg_transformer_t = std::conditional_t<std::same_as<D, std::string_view>, std::string, D>;
    } // namespace impl

    template <typename T, typename Args>
    static constexpr auto can_apply_v = impl::can_apply<T, Args>::value;

    template <typename T, typename Args>
    using apply_result_t = impl::apply_result<T, Args>::type;

    template <typename T>
    static constexpr auto has_reference_v = impl::has_reference<T>::value;

    template <typename T>
    using raw_args_t = boost::callable_traits::args_t<T>;

    template <typename T, template <typename...> typename Transform = impl::arg_transformer_t>
    using args_t = tuple::transform_t<raw_args_t<T>, Transform>;

    template <typename T>
    using result_t = boost::callable_traits::return_type_t<T>;

    template <typename T,                                                                                                //
              typename Args,                                                                                             //
              typename Executor,                                                                                         //
              bool TakesExecutor = can_apply_v<T, tuple::add_t<Args, Executor>>,                                         //
              typename Result = apply_result_t<T, std::conditional_t<TakesExecutor, tuple::add_t<Args, Executor>, Args>> //
              >
    struct converter;

    template <typename T, typename Args, typename Executor>
    struct converter<T, Args, Executor, true, void>
    {
        static decltype(auto) convert(T callable)
        {
            return callable;
        }
    };

    template <typename T, typename... Ts, typename R, typename E, typename Result>
    struct converter<T, std::tuple<Ts...>, executor<R, E>, false, Result>
    {
        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)](Ts &&...args, auto &&executor) mutable
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
    struct converter<T, std::tuple<Ts...>, executor<R, E>, false, std::expected<R, E>>
    {
        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)](Ts &&...args, auto &&executor) mutable
            {
                std::invoke(callable, std::forward<Ts>(args)...)
                    .transform(executor.resolve)
                    .transform_error([&executor]<typename... Us>(Us &&...args)
                                     { return std::invoke(executor.reject, std::forward<Us>(args)...), 0; });
            };
        }
    };

    template <typename T, typename Result = result_t<T>, typename Last = tuple::last_t<args_t<T>>>
        requires(!has_reference_v<raw_args_t<T>>)
    struct resolver
    {
        using args     = args_t<T>;
        using error    = void;
        using result   = Result;
        using executor = saucer::executor<Result, void>;

      public:
        using converter = traits::converter<T, args, executor>;
    };

    template <typename T, typename Result, typename R, typename E>
    struct resolver<T, Result, executor<R, E>>
    {
        using args     = tuple::drop_last_t<args_t<T>>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, args, executor>;
    };

    template <typename T, typename R, typename E, typename Last>
    struct resolver<T, std::expected<R, E>, Last>
    {
        using args     = args_t<T>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, args, executor>;
    };
} // namespace saucer::traits
