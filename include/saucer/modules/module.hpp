#pragma once

#include <concepts>
#include <type_traits>

#include <optional>
#include <unordered_map>

#include <eraser/erased.hpp>

namespace saucer
{
    template <typename T>
    struct stable_natives;

    template <typename T, bool Stable>
    using natives = std::conditional_t<Stable, stable_natives<T>, typename T::impl *>;

    template <typename S, typename T, typename... Ts>
    concept Module = std::constructible_from<S, T *, Ts...>;

    template <typename T, typename Interface = eraser::interface<>>
    class extensible
    {
        using module_map = std::unordered_map<std::size_t, eraser::erased<Interface>>;

      private:
        T *m_class;
        module_map m_modules;

      public:
        extensible(T *clazz);

      protected:
        auto modules() const;

      public:
        template <typename M, typename... Ts>
            requires Module<M, T, Ts...>
        M &add_module(Ts &&...);

      public:
        template <typename M>
        [[nodiscard]] std::optional<M *> module();
    };
} // namespace saucer

#include "module.inl"
