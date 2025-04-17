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
        template <bool Success, typename T = void>
        struct apply_info
        {
            using type                    = T;
            static constexpr auto success = Success;
        };

        template <typename T, typename Args>
        struct apply
        {
            using type = apply_info<false>;
        };

        template <typename T, typename... Ts>
            requires std::invocable<T, Ts...>
        struct apply<T, std::tuple<Ts...>>
        {
            using type = apply_info<true, std::invoke_result_t<T, Ts...>>;
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

    using apply_failure = impl::apply_info<false>;

    template <typename T>
    using apply_success = impl::apply_info<true, T>;

    template <typename T, typename Args>
    using apply_t = impl::apply<T, Args>::type;

    template <typename T>
    static constexpr auto has_reference_v = impl::has_reference<T>::value;

    template <typename T>
    using raw_args_t = boost::callable_traits::args_t<T>;

    template <typename T, template <typename...> typename Transform = impl::arg_transformer_t>
    using args_t = tuple::transform_t<raw_args_t<T>, Transform>;

    template <typename T>
    using result_t = boost::callable_traits::return_type_t<T>;

    template <typename T,                                                                                                 //
              typename Args,                                                                                              //
              typename Executor,                                                                                          //
              typename WithExecutor = apply_t<T, tuple::add_t<Args, Executor>>,                                           //
              typename Result = apply_t<T, std::conditional_t<WithExecutor::success, tuple::add_t<Args, Executor>, Args>> //
              >
    struct converter
    {
        static_assert(std::same_as<WithExecutor, apply_failure> && std::same_as<Result, apply_failure>,
                      "Could not match arguments. Make sure you are using appropiate value categories!");
    };

    template <typename T, typename Args, typename Executor, typename _>
    struct converter<T, Args, Executor, apply_success<_>, apply_success<void>>
    {
        static decltype(auto) convert(T callable)
        {
            return callable;
        }
    };

    template <typename T, typename... Ts, typename R, typename E, typename Result>
    struct converter<T, std::tuple<Ts...>, executor<R, E>, apply_failure, apply_success<Result>>
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
    struct converter<T, std::tuple<Ts...>, executor<R, E>, apply_failure, apply_success<std::expected<R, E>>>
    {
        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)](Ts &&...args, auto &&executor) mutable
            {
                auto result = std::invoke(callable, std::forward<Ts>(args)...);

                if (!result.has_value())
                {
                    std::invoke(executor.reject, result.error());
                    return;
                }

                if constexpr (std::is_void_v<R>)
                {
                    std::invoke(executor.resolve);
                }
                else
                {
                    std::invoke(executor.resolve, result.value());
                }
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
