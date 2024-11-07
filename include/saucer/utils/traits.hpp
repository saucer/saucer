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

        template <typename T, typename... Ts>
        consteval auto apply_result(const std::tuple<Ts...> &) -> std::invoke_result_t<T, Ts...>;

        template <typename T, typename... Ts>
        consteval auto can_apply(const std::tuple<Ts...> &) -> std::bool_constant<std::invocable<T, Ts...>>;

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

    template <typename T, typename Args>
    using apply_result_t = decltype(impl::apply_result<T>(std::declval<Args>()));

    template <typename T, typename Args>
    concept can_apply = decltype(impl::can_apply<T>(std::declval<Args>()))::value;

    template <typename T>
    using drop_last_t = decltype(impl::drop_last(std::declval<T>()))::type;

    template <typename T>
    using last_t = decltype(impl::last(std::declval<T>()))::type;

    template <typename T, typename O>
    using tuple_cat_t = decltype(std::tuple_cat(std::declval<T>(), std::declval<O>()));

    template <typename T, template <typename...> typename Transform = std::decay_t>
    using args_t = transform_tuple_t<boost::callable_traits::args_t<T>, Transform>;

    template <typename T>
    using result_t = boost::callable_traits::return_type_t<T>;

    template <typename T,                                                                          //
              typename Executor,                                                                   //
              typename Args,                                                                       //
              typename ExtArgs = tuple_cat_t<Args, std::tuple<Executor>>,                          //
              auto HasExecutor = can_apply<T, ExtArgs>,                                            //
              typename Result  = apply_result_t<T, std::conditional_t<HasExecutor, ExtArgs, Args>> //
              >
    struct converter
    {
        static decltype(auto) convert(T &&callable)
            requires std::same_as<Result, typename Executor::result>
        {
            return impl::make_callable<Executor, Args>(
                [callable = std::forward<T>(callable)]<typename... Ts>(auto &&executor, Ts &&...args)
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
                });
        }
    };

    template <typename T,        //
              typename Executor, //
              typename Args,     //
              typename ExtArgs   //
              >
    struct converter<T, Executor, Args, ExtArgs, true, void>
    {
        static decltype(auto) convert(T &&callable)
        {
            return std::forward<T>(callable);
        }
    };

    template <typename T,            //
              typename Executor,     //
              typename Args,         //
              typename ExtArgs,      //
              typename R, typename E //
              >
    struct converter<T, Executor, Args, ExtArgs, false, std::expected<R, E>>
    {
        static decltype(auto) convert(T &&callable)
            requires std::same_as<R, typename Executor::result> and std::same_as<E, typename Executor::error>
        {
            return impl::make_callable<Executor, Args>(
                [callable = std::forward<T>(callable)]<typename... Ts>(auto &&executor, Ts &&...args)
                {
                    [[maybe_unused]] auto result =
                        std::invoke(callable, std::forward<Ts>(args)...).transform(executor.resolve);

                    if (result.has_value())
                    {
                        return;
                    }

                    if constexpr (std::is_void_v<E>)
                    {
                        std::invoke(executor.reject);
                    }
                    else
                    {
                        std::invoke(executor.reject, result.error());
                    }
                });
        }
    };

    template <typename T, typename Result = result_t<T>, typename Last = last_t<args_t<T>>>
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
        using args     = drop_last_t<args_t<T>>;
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
