#pragma once

#include <utility>
#include <tuple>

#include <type_traits>
#include <cstddef>

namespace saucer::tuple
{
    namespace impl
    {
        template <typename T>
        struct is_tuple : std::false_type
        {
        };

        template <typename... Ts>
        struct is_tuple<std::tuple<Ts...>> : std::true_type
        {
        };

        template <template <typename...> typename Transform, typename... Ts>
        consteval auto transform(std::tuple<Ts...>) -> std::tuple<Transform<Ts>...>;

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
    } // namespace impl

    template <typename T>
    concept Tuple = impl::is_tuple<T>::value;

    template <typename T, template <typename...> typename Transform>
    using transform_t = decltype(impl::transform<Transform>(std::declval<T>()));

    template <typename T>
    using drop_last_t = decltype(impl::drop_last(std::declval<T>()))::type;

    template <typename T>
    using last_t = decltype(impl::last(std::declval<T>()))::type;

    template <typename T, typename O>
    using cat_t = decltype(std::tuple_cat(std::declval<T>(), std::declval<O>()));

    template <typename T, typename... Ts>
    using add_t = cat_t<T, std::tuple<Ts...>>;
} // namespace saucer::tuple
