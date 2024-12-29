#pragma once

#include "module.hpp"

#include <ranges>

namespace saucer
{
    template <typename T, typename Interface>
    extensible<T, Interface>::extensible(T *parent) : m_parent(parent)
    {
    }

    template <typename T, typename Interface>
    auto extensible<T, Interface>::modules() const
    {
        return std::views::values(m_modules);
    }

    template <typename T, typename Interface>
    template <typename M, typename... Ts>
        requires Module<M, T, Ts...>
    M &extensible<T, Interface>::add_module(Ts &&...args)
    {
        using eraser::make_erased;

        const auto id      = eraser::id_of<M>();
        const auto [it, _] = m_modules.emplace(id, make_erased<Interface, M>(m_parent, std::forward<Ts>(args)...));

        return *it->second.template as<M>().value();
    }

    template <typename T, typename Interface>
    template <typename M>
    std::optional<M *> extensible<T, Interface>::module()
    {
        const auto key = eraser::id_of<M>();

        if (!m_modules.contains(key))
        {
            return std::nullopt;
        }

        return m_modules.at(key).template as<M>();
    }
} // namespace saucer
