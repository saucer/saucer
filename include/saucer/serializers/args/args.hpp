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
        template <typename... Fs>
        explicit arguments(Fs &&...args) : std::tuple<Ts...>{std::forward<Fs>(args)...}
        {
        }

      public:
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] const std::tuple<Ts...> &as_tuple() const;
    };

    namespace impl
    {
        template <typename T>
        struct is_arguments_impl : std::false_type
        {
        };

        template <typename... Ts>
        struct is_arguments_impl<arguments<Ts...>> : std::true_type
        {
        };
    } // namespace impl

    template <typename... Ts>
    auto make_args(Ts &&...);

    template <typename T>
    concept is_arguments = requires() { requires impl::is_arguments_impl<T>::value; };
} // namespace saucer

#include "args.inl"
