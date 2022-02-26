#pragma once
#include "variable.hpp"

namespace saucer
{
    template <typename T> template <typename... Args> variable<T>::variable(Args &&...args) : m_value(std::forward<Args>(args)...) {}

    template <typename T> template <typename, typename> void variable<T>::assign(std::decay_t<T> &&value)
    {
        m_value = std::move(value);
        m_events.template at<variable_event::updated>().fire();
    }
    template <typename T> template <typename, typename> void variable<T>::assign(const std::decay_t<T> &value)
    {
        m_value = value;
        m_events.template at<variable_event::updated>().fire();
    }

    template <typename T> proxy<T> variable<T>::modify()
    {
        return proxy<T>(m_value, *this);
    }

    template <typename T> const T &variable<T>::read() const
    {
        return m_value;
    }

    template <typename T> void variable<T>::clear(variable_event event)
    {
        m_events.clear(event);
    }

    template <typename T> void variable<T>::unregister(variable_event event, std::size_t id)
    {
        m_events.unregister(event, id);
    }

    template <typename T> template <variable_event Event> std::size_t variable<T>::on(typename events::template get_t<Event> &&callback)
    {
        return m_events.template at<Event>().add_callback(std::move(callback));
    }

    template <typename T> proxy<T>::proxy(T &value, variable<T> &parent) : m_value(value), m_parent(parent) {}
    template <typename T> proxy<T>::~proxy()
    {
        m_parent.m_events.template at<variable_event::updated>().fire();
    }

    template <typename T> std::add_lvalue_reference_t<T> proxy<T>::operator*() noexcept
    {
        return m_value;
    }
    template <typename T> std::add_lvalue_reference_t<T> proxy<T>::operator*() const noexcept
    {
        return m_value;
    }

    template <typename T> std::add_pointer_t<T> proxy<T>::operator->() noexcept
    {
        return &m_value;
    }
    template <typename T> std::add_pointer_t<T> proxy<T>::operator->() const noexcept
    {
        return &m_value;
    }
} // namespace saucer