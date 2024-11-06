#pragma once

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
        template <template <typename...> typename Transform, typename... Ts>
        consteval auto transform_tuple(std::tuple<Ts...>) -> std::tuple<Transform<Ts>...>;

        template <std::size_t Count, typename... Ts, typename Tuple = std::tuple<Ts...>>
        consteval auto extract(const Tuple &)
        {
            auto unpack = []<auto... Is>(std::index_sequence<Is...>)
            {
                return std::type_identity<std::tuple<std::tuple_element_t<Is, Tuple>...>>{};
            };

            return unpack(std::make_index_sequence<Count>());
        }

        template <typename... Ts, typename Tuple = std::tuple<Ts...>, auto Size = std::tuple_size_v<Tuple>>
        consteval auto drop_last(const Tuple &value)
        {
            static constexpr auto count = Size <= 1 ? 0 : Size - 1;
            return extract<count>(value);
        }

        template <typename... Ts, typename Tuple = std::tuple<Ts...>, auto Size = std::tuple_size_v<Tuple>>
        consteval auto last(const Tuple &)
        {
            if constexpr (Size >= 1)
            {
                return std::type_identity<std::tuple_element_t<Size - 1, Tuple>>{};
            }
            else
            {
                return std::type_identity<void>{};
            }
        }

        template <typename Executor, typename Args, typename Callable>
        constexpr auto make_callable(Callable &&callable)
        {
            auto unpack = [&]<auto... Is>(std::index_sequence<Is...>)
            {
                return [callable = std::forward<Callable>(callable)](std::tuple_element_t<Is, Args>... args,
                                                                     const Executor &executor)
                {
                    return std::invoke(callable, executor, std::forward<decltype(args)>(args)...);
                };
            };

            return unpack(std::make_index_sequence<std::tuple_size_v<Args>>());
        }
    } // namespace impl

    template <typename T, template <typename...> typename Transform>
    using transform_tuple_t = decltype(impl::transform_tuple<Transform>(std::declval<T>()));

    template <std::size_t Count, typename T>
    using extract_t = decltype(impl::extract<Count>(std::declval<T>()))::type;

    template <typename T>
    using drop_last_t = decltype(impl::drop_last(std::declval<T>()))::type;

    template <typename T>
    using last_t = decltype(impl::last(std::declval<T>()))::type;

    template <typename T, template <typename...> typename Transform = std::decay_t>
    using args_t = transform_tuple_t<boost::callable_traits::args_t<T>, Transform>;

    template <typename T>
    using result_t = boost::callable_traits::return_type_t<T>;

    template <typename T, typename Result = result_t<T>, typename Last = last_t<args_t<T>>>
    struct resolver
    {
        using args_t     = args_t<T>;
        using error_t    = void;
        using result_t   = Result;
        using executor_t = executor<Result, void>;

      public:
        static decltype(auto) convert(T &&callable)
        {
            return impl::make_callable<executor_t, args_t>(
                [callable = std::forward<T>(callable)]<typename... Ts>(auto &&executor, Ts &&...args)
                { std::invoke(executor.resolve, std::invoke(callable, std::forward<Ts>(args)...)); });
        }
    };

    template <typename T, typename Result, typename R, typename E>
    struct resolver<T, Result, executor<R, E>>
    {
        using args_t     = drop_last_t<args_t<T>>;
        using error_t    = E;
        using result_t   = R;
        using executor_t = executor<R, E>;

      public:
        static decltype(auto) convert(T &&callable)
        {
            return callable;
        }
    };

    template <typename T, typename R, typename E, typename Last>
    struct resolver<T, std::expected<R, E>, Last>
    {
        using args_t     = args_t<T>;
        using error_t    = E;
        using result_t   = R;
        using executor_t = executor<R, E>;

      public:
        static decltype(auto) convert(T &&callable)
        {
            return impl::make_callable<executor_t, args_t>(
                [callable = std::forward<T>(callable)]<typename... Ts>(auto &&executor, Ts &&...args) {
                    std::invoke(callable, std::forward<Ts>(args)...)
                        .transform(executor.resolve)
                        .transform_error(executor.reject);
                });
        }
    };
} // namespace saucer::traits
