#pragma once

#include <cstddef>
#include <utility>

#include <tuple>
#include <type_traits>

namespace saucer
{
    template <typename... Ts>
    struct arguments : std::tuple<Ts...>
    {
        using underlying = std::tuple<Ts...>;

      public:
        template <typename... Fs>
        explicit arguments(Fs &&...args) : std::tuple<Ts...>(std::forward<Fs>(args)...)
        {
        }

      public:
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] const underlying &as_tuple() const;
    };

    namespace impl
    {
        template <typename T>
        struct is_arguments : std::false_type
        {
        };

        template <typename... Ts>
        struct is_arguments<arguments<Ts...>> : std::true_type
        {
        };
    } // namespace impl

    template <typename... Ts>
    auto make_args(Ts &&...);

    template <typename T>
    concept is_arguments = requires() { requires impl::is_arguments<T>::value; };
} // namespace saucer

#include "args.inl"
