#pragma once

#include <type_traits>
#include <unordered_map>

#include <optional>
#include <functional>

namespace saucer
{
    template <typename T>
    struct stable;

    template <typename T, bool Stable>
    using natives = std::conditional_t<Stable, stable<T>, typename T::impl *>;

    template <typename S, typename T, typename... Ts>
    concept Module = requires() { requires std::constructible_from<S, T *, Ts...>; };

    class erased_module
    {
        std::size_t m_id;
        std::function<void *()> m_value;

      private:
        erased_module() = default;

      public:
        template <typename T>
        [[nodiscard]] std::optional<T *> get() const;

      public:
        template <typename T>
        [[nodiscard]] static std::size_t id_of();

      public:
        template <typename T, typename... Ts>
        [[nodiscard]] static erased_module from(Ts &&...args);
    };

    template <typename T>
    class extensible
    {
        T *m_parent;
        std::unordered_map<std::size_t, erased_module> m_modules;

      public:
        extensible(T *parent);

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
