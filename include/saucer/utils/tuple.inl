#pragma once

#include "tuple.hpp"

#include <tuple>
#include <type_traits>

namespace saucer::tuple
{
    template <typename T>
    struct is_tuple : std::false_type
    {
    };

    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type
    {
    };

    template <typename... Ts, template <typename...> typename Transform>
    struct transform<std::tuple<Ts...>, Transform>
    {
        using type = std::tuple<Transform<Ts>...>;
    };

    template <typename T>
        requires(std::tuple_size_v<T> == 0)
    struct drop_last<T>
    {
        using type = std::tuple<>;
    };

    template <typename T>
        requires(std::tuple_size_v<T> > 0)
    struct drop_last<T>
    {
        template <auto... Is>
        static constexpr auto unpack(std::index_sequence<Is...>) -> std::tuple<std::tuple_element_t<Is, T>...>;

      public:
        using type = decltype(unpack(std::make_index_sequence<std::tuple_size_v<T> - 1>()));
    };

    template <typename T>
        requires(std::tuple_size_v<T> == 0)
    struct last<T>
    {
        using type = void;
    };

    template <typename T>
        requires(std::tuple_size_v<T> > 0)
    struct last<T>
    {
        using type = std::tuple_element_t<std::tuple_size_v<T> - 1, T>;
    };
} // namespace saucer::tuple
