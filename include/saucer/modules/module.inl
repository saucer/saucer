#pragma once

#include "module.hpp"

#include <atomic>

namespace saucer
{
    namespace impl
    {
        inline auto count()
        {
            static std::atomic_size_t counter{};
            return counter++;
        }

        template <typename T>
        struct id
        {
            static const inline auto value = count();
        };
    } // namespace impl

    template <typename T>
    std::optional<T *> erased_module::get() const
    {
        if (id_of<T>() != m_id)
        {
            return std::nullopt;
        }

        return reinterpret_cast<T *>(m_value());
    }

    template <typename T>
    std::size_t erased_module::id_of()
    {
        return impl::id<T>::value;
    }

    template <typename T, typename... Ts>
    erased_module erased_module::from(Ts &&...args)
    {
        erased_module rtn;

        // std::function is often implemented with SBO, we're abusing this here so that we don't have to handle that logic
        // ourselves.

        rtn.m_id    = id_of<T>();
        rtn.m_value = [storage = T{std::forward<Ts>(args)...}]() mutable
        {
            return &storage;
        };

        return rtn;
    }

    template <typename T>
    extensible<T>::extensible(T *parent) : m_parent(parent)
    {
    }

    template <typename T>
    template <typename M, typename... Ts>
        requires Module<M, T, Ts...>
    M &extensible<T>::add_module(Ts &&...args)
    {
        const auto id      = erased_module::id_of<M>();
        const auto [it, _] = m_modules.emplace(id, erased_module::from<M>(m_parent, std::forward<Ts>(args)...));

        return *it->second.template get<M>().value();
    }

    template <typename T>
    template <typename M>
    std::optional<M *> extensible<T>::module()
    {
        const auto key = erased_module::id_of<M>();

        if (!m_modules.contains(key))
        {
            return std::nullopt;
        }

        return m_modules.at(key).template get<M>();
    }
} // namespace saucer
