#pragma once

#include <tuple>
#include <type_traits>

namespace saucer
{
    namespace impl
    {
        template <typename T>
        struct is_arguments;
    } // namespace impl

    template <typename... Ts>
    class arguments
    {
        std::tuple<Ts...> m_tuple;

      public:
        explicit arguments(Ts...);

      public:
        [[nodiscard]] constexpr auto &tuple();
    };

    template <typename... Ts>
    constexpr auto make_args(Ts &&...);

    template <typename T>
    concept Arguments = impl::is_arguments<std::remove_cvref_t<T>>::value;
} // namespace saucer

#include "args.inl"
