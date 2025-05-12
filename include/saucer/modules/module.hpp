#pragma once

#include <type_traits>
#include <concepts>

#include <unordered_map>
#include <optional>
#include <string>

#include <eraser/erased.hpp>

namespace saucer
{
    namespace modules
    {
        struct webview_methods
        {
            static inline auto on_message = [](auto &self, const auto &message)
            {
                if constexpr (requires { self.on_message(message); })
                {
                    return self.on_message(message);
                }
                else
                {
                    return false;
                }
            };
        };

        using webview = eraser::interface<eraser::method<0, webview_methods::on_message, bool(const std::string &)>>;
    } // namespace modules

    template <typename T>
    struct stable_natives;

    template <typename T, bool Stable>
    using natives = std::conditional_t<Stable, stable_natives<T>, typename T::impl *>;

    template <typename S, typename T, typename... Ts>
    concept Module = std::constructible_from<S, T *, Ts...>;

    template <typename T, typename Interface = eraser::interface<>>
    class extensible
    {
        friend T;

      private:
        using module_map = std::unordered_map<std::size_t, eraser::erased<Interface>>;

      private:
        T *m_parent;
        module_map m_modules;

      public:
        extensible(T *parent);

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
