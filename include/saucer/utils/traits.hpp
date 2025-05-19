#pragma once

#include "tuple.hpp"
#include "../executor.hpp"

#include <utility>
#include <type_traits>

#include <tuple>
#include <expected>

#include <coco/utils/utils.hpp>
#include <coco/traits/traits.hpp>

namespace saucer::traits
{
    namespace impl
    {
        template <typename T>
        struct function_traits : function_traits<decltype(std::function{std::declval<T>()})>
        {
        };

        template <typename R, typename... Ts>
        struct function_traits<std::function<R(Ts...)>>
        {
            using result = R;
            using args   = std::tuple<Ts...>;

          public:
            using signature = R(Ts...);
        };

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

    template <typename T>
    using raw_args_t = impl::function_traits<T>::args;

    template <typename T>
    concept NonRefCallable = !impl::has_reference<raw_args_t<T>>::value;

    template <typename T, template <typename...> typename Transform = impl::arg_transformer_t>
    using args_t = tuple::transform_t<raw_args_t<T>, Transform>;

    template <typename T>
    using signature_t = impl::function_traits<T>::signature;

    template <typename T>
    using result_t = impl::function_traits<T>::result;

    using apply_failure = impl::apply_info<false>;

    template <typename T>
    using apply_success = impl::apply_info<true, T>;

    template <typename T, typename Args>
    using apply_t = impl::apply<T, Args>::type;

    template <typename T,                                                       //
              typename Args,                                                    //
              typename Executor,                                                //
              typename WithExecutor = apply_t<T, tuple::add_t<Args, Executor>>, //
              typename Result       = apply_t<T, std::conditional_t<WithExecutor::success, tuple::add_t<Args, Executor>, Args>>>
    struct converter
    {
        static_assert(std::same_as<WithExecutor, apply_failure> && std::same_as<Result, apply_failure>,
                      "Could not find appropiate converter. Make sure to return `void` when taking an executor!");
    };

    template <NonRefCallable T, typename Result = result_t<T>, typename Last = tuple::last_t<args_t<T>>>
    struct resolver
    {
        using args     = args_t<T>;
        using error    = void;
        using result   = Result;
        using executor = saucer::executor<Result, void>;

      public:
        using converter = traits::converter<T, args, executor, apply_failure, apply_success<Result>>;
    };

    template <NonRefCallable T, typename Result, typename R, typename E>
    struct resolver<T, Result, executor<R, E>>
    {
        using args     = tuple::drop_last_t<args_t<T>>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, args, executor, apply_success<Result>, apply_success<Result>>;
    };

    template <NonRefCallable T, typename R, typename E, typename Last>
    struct resolver<T, std::expected<R, E>, Last>
    {
        using args     = args_t<T>;
        using error    = E;
        using result   = R;
        using executor = saucer::executor<R, E>;

      public:
        using converter = traits::converter<T, args, executor, apply_failure, apply_success<std::expected<R, E>>>;
    };

    template <NonRefCallable T, coco::Awaitable Result, typename Last>
    struct resolver<T, Result, Last>
    {
        using underlying = resolver<T, typename coco::traits<Result>::result, Last>;

      public:
        using args     = underlying::args;
        using error    = underlying::error;
        using result   = underlying::result;
        using executor = underlying::executor;

      public:
        using converter = traits::converter<T, args, executor, apply_failure, apply_success<Result>>;
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
        template <typename... Rs>
        static void resolve(auto &&executor, Rs &&...result)
        {
            std::invoke(executor.resolve, std::forward<Rs>(result)...);
        }

        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)]<typename Executor>(Ts &&...args, Executor &&executor) mutable
            {
                if constexpr (std::is_void_v<Result>)
                {
                    std::invoke(callable, std::forward<Ts>(args)...);
                    resolve(std::forward<Executor>(executor));
                }
                else
                {
                    resolve(std::forward<Executor>(executor), std::invoke(callable, std::forward<Ts>(args)...));
                }
            };
        }
    };

    template <typename T, typename... Ts, typename R, typename E>
    struct converter<T, std::tuple<Ts...>, executor<R, E>, apply_failure, apply_success<std::expected<R, E>>>
    {
        static void resolve(auto &&executor, auto &&result)
        {
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
        }

        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)]<typename Executor>(Ts &&...args, Executor &&executor) mutable
            {
                resolve(std::forward<Executor>(executor), std::invoke(callable, std::forward<Ts>(args)...));
            };
        }
    };

    template <typename T, typename... Ts, typename R, typename E, coco::Awaitable Result>
    struct converter<T, std::tuple<Ts...>, executor<R, E>, apply_failure, apply_success<Result>>
    {
        template <typename Executor, typename... Rs>
        static void resolve(Executor &&executor, Rs &&...result)
        {
            using result_t    = coco::traits<Result>::result;
            using converter_t = resolver<T, result_t>::converter;

            static_assert(not std::same_as<converter_t, converter>);

            converter_t::resolve(std::forward<Executor>(executor), std::forward<Rs>(result)...);
        }

        static decltype(auto) convert(T callable)
        {
            return [callable = std::move(callable)]<typename Executor>(Ts &&...args, Executor &&executor) mutable
            {
                auto callback = [executor = std::forward<Executor>(executor)]<typename... Rs>(Rs &&...result) mutable
                {
                    resolve(std::move(executor), std::forward<Rs>(result)...);
                };

                coco::then(std::invoke(callable, std::forward<Ts>(args)...), std::move(callback));
            };
        }
    };
} // namespace saucer::traits
