#pragma once

#include "lease.hpp"
#include "invoke.hpp"

#include <mutex>
#include <optional>

namespace saucer::utils
{
    template <typename T>
    struct lease<T>::state
    {
        std::optional<T> value;
        std::shared_mutex mutex;
    };

    template <typename T>
    lease<T>::lease() = default;

    template <typename T>
    lease<T>::lease(T value) : m_state(std::make_shared<state>(std::move(value)))
    {
    }

    template <typename T>
    lease<T>::lease(lease &&other) noexcept = default;

    template <typename T>
    lease<T> &lease<T>::operator=(lease &&other) noexcept = default;

    template <typename T>
    lease<T>::~lease()
    {
        if (!m_state)
        {
            return;
        }

        std::unique_lock guard{m_state->mutex};

        m_state->value.reset();
    }

    template <typename T>
    T &lease<T>::value() &
    {
        return *m_state->value;
    }

    template <typename T>
    rental<T> lease<T>::rent() const
    {
        return rental<T>{*this};
    }

    template <typename T>
    rental<T>::rental() = default;

    template <typename T>
    rental<T>::rental(const lease<T> &other) : m_state(other.m_state)
    {
    }

    template <typename T>
    rental<T>::rental(const rental &) = default;

    template <typename T>
    rental<T>::rental(rental &&) noexcept = default;

    template <typename T>
    rental<T>::lock rental<T>::access() const
    {
        return lock{*this};
    }

    template <typename T>
    rental<T>::lock::lock(const rental &other) : m_state(other.m_state), m_guard(m_state->mutex)
    {
    }

    template <typename T>
    T *rental<T>::lock::value() const
    {
        auto &rtn = m_state->value;

        if (!rtn.has_value())
        {
            return nullptr;
        }

        return std::addressof(*rtn);
    }

    template <typename T, typename Func>
    auto defer(lease<T> &lease, Func &&func)
    {
        return [rental = lease.rent(), func = std::forward<Func>(func)]<typename... Ts>(Ts &&...args) mutable
        {
            auto locked = rental.access();
            auto value  = locked.value();

            if (!value)
            {
                return detail::noop<std::invoke_result_t<Func, T, Ts...>>();
            }

            return func(*value, std::forward<Ts>(args)...);
        };
    }
} // namespace saucer::utils
