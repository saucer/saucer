#pragma once

#include "traits.hpp"

#include "../executor.hpp"
#include "../utils/tuple.hpp"

#include <expected>
#include <functional>

#include <concepts>
#include <type_traits>

#include <coco/utils/utils.hpp>
#include <coco/traits/traits.hpp>

namespace saucer::traits
{
    namespace detail
    {
        template <typename T>
        struct callable;

        template <typename T, typename E, typename... Ts>
        void safe_invoke(E &&, T &&, Ts &&...);

        template <typename T, typename Args, typename Executor>
        struct awaitable;

        template <typename T, typename Args, typename Executor>
        static constexpr auto awaitable_v = awaitable<T, Args, Executor>::value;

        template <typename T, typename D = std::remove_cvref_t<T>>
        using fix_arg_t = std::conditional_t<std::same_as<D, std::string_view>, std::string, D>;

        template <typename T>
        using fixed_args_t = tuple::transform_t<typename function_traits<T>::args, fix_arg_t>;

        template <typename T>
        using result_t = function_traits<T>::result;

        template <typename T, typename Args = fixed_args_t<T>, typename Result = result_t<T>, typename Last = tuple::last_t<Args>>
        struct resolver;
    } // namespace detail

    template <typename T>
    struct detail::callable
    {
        T callable;
    };

    template <typename T, typename E, typename... Ts>
    void detail::safe_invoke(E &&except, T &&fn, Ts &&...args)
#if defined(__cpp_exceptions) && !defined(SAUCER_NO_EXCEPTIONS)
    try
#endif
    {
        std::invoke(std::forward<T>(fn), std::forward<Ts>(args)...);
    }
#if defined(__cpp_exceptions) && !defined(SAUCER_NO_EXCEPTIONS)
    catch (...)
    {
        std::invoke(std::forward<E>(except), std::current_exception());
    }
#endif

    template <typename T, typename Args, typename Executor>
    struct detail::awaitable
    {
        static constexpr auto value = false;
    };

    template <typename T, typename... Ts, typename Executor>
        requires coco::awaitable<std::invoke_result_t<T, Ts...>>
    struct detail::awaitable<T, std::tuple<Ts...>, Executor>
    {
        using result      = std::invoke_result_t<T, Ts...>;
        using transformer = traits::transformer<mock_return<T, typename coco::traits<result>::result>, std::tuple<Ts...>, Executor>;

      public:
        static constexpr auto value = transformer::valid;
    };

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

    template <typename T, typename R>
    struct mock_return
    {
        template <typename... Ts>
            requires std::invocable<T, Ts...>
        static R operator()(Ts &&...);
    };

    template <typename T, typename Args, typename Executor>
    struct transformer
    {
        static constexpr auto valid = false;
    };

    template <typename T, typename... Ts, typename Executor>
        requires std::invocable<T, Ts..., Executor>
    struct transformer<T, std::tuple<Ts...>, Executor> : detail::callable<T>
    {
        static constexpr auto valid = true;

      public:
        void operator()(Ts... args, Executor executor)
        {
            std::invoke(this->callable, std::forward<Ts>(args)..., std::move(executor));
        }

        template <typename F>
        void operator()(F &&except, Ts... args, Executor executor)
        {
            detail::safe_invoke(std::forward<F>(except), *this, std::forward<Ts>(args)..., std::move(executor));
        }
    };

    template <typename T, typename... Ts, typename R, typename E>
        requires std::same_as<std::invoke_result_t<T, Ts...>, R>
    struct transformer<T, std::tuple<Ts...>, executor<R, E>> : detail::callable<T>
    {
        static constexpr auto valid = true;

      public:
        template <typename... Rs>
        static void resolve(auto &&executor, Rs &&...result)
        {
            std::invoke(executor.resolve, std::forward<Rs>(result)...);
        }

      public:
        void operator()(Ts... args, executor<R, E> executor)
            requires std::is_void_v<R>
        {
            std::invoke(this->callable, std::forward<Ts>(args)...);
            resolve(std::move(executor));
        }

        void operator()(Ts... args, executor<R, E> executor)
            requires(not std::is_void_v<R>)
        {
            resolve(std::move(executor), std::invoke(this->callable, std::forward<Ts>(args)...));
        }

        template <typename F>
        void operator()(F &&except, Ts... args, executor<R, E> executor)
        {
            detail::safe_invoke(std::forward<F>(except), *this, std::forward<Ts>(args)..., std::move(executor));
        }
    };

    template <typename T, typename... Ts, typename R, typename E>
        requires std::same_as<std::invoke_result_t<T, Ts...>, std::expected<R, E>>
    struct transformer<T, std::tuple<Ts...>, executor<R, E>> : detail::callable<T>
    {
        static constexpr auto valid = true;

      public:
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

      public:
        void operator()(Ts... args, executor<R, E> executor)
        {
            resolve(std::move(executor), std::invoke(this->callable, std::forward<Ts>(args)...));
        }

        template <typename F>
        void operator()(F &&except, Ts... args, executor<R, E> executor)
        {
            detail::safe_invoke(std::forward<F>(except), *this, std::forward<Ts>(args)..., std::move(executor));
        }
    };

    template <typename T, typename... Ts, typename Executor>
        requires detail::awaitable_v<T, std::tuple<Ts...>, Executor>
    struct transformer<T, std::tuple<Ts...>, Executor> : detail::callable<T>
    {
        static constexpr auto valid = true;

      public:
        template <typename U, typename... Rs>
        static void resolve(U &&executor, Rs &&...result)
        {
            using base = detail::awaitable<T, std::tuple<Ts...>, Executor>::transformer;
            base::resolve(std::forward<U>(executor), std::forward<Rs>(result)...);
        }

      public:
        template <typename... F>
        void operator()(Ts... args, Executor executor, F... except)
        {
            auto fn = [executor = std::move(executor)]<typename... Rs>(Rs &&...result) mutable
            {
                resolve(std::move(executor), std::forward<Rs>(result)...);
            };

            coco::then(std::invoke(this->callable, std::forward<Ts>(args)...), std::move(fn), std::move(except)...);
        }

        template <typename F>
        void operator()(F except, Ts... args, Executor executor)
        {
            detail::safe_invoke(except, *this, std::forward<Ts>(args)..., std::move(executor), except);
        }
    };

    template <typename T, typename Args, typename Result, typename Last>
    struct detail::resolver
    {
        using args        = Args;
        using executor    = saucer::executor<Result, void>;
        using transformer = traits::transformer<T, args, executor>;
    };

    template <typename T, typename Args, typename R, typename E, typename Last>
    struct detail::resolver<T, Args, std::expected<R, E>, Last>
    {
        using args        = Args;
        using executor    = saucer::executor<R, E>;
        using transformer = traits::transformer<T, args, executor>;
    };

    template <typename T, typename Args, typename R, typename E>
    struct detail::resolver<T, Args, void, executor<R, E>>
    {
        using args        = tuple::drop_last_t<Args>;
        using executor    = saucer::executor<R, E>;
        using transformer = traits::transformer<T, args, executor>;
    };

    template <typename T, typename Args, coco::awaitable Result, typename Last>
    struct detail::resolver<T, Args, Result, Last> : detail::resolver<T, Args, typename coco::traits<Result>::result, Last>
    {
    };

    template <typename T>
    struct resolver : detail::resolver<T>
    {
    };
} // namespace saucer::traits
