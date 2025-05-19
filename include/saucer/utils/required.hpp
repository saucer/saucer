#pragma once

#include <memory>
#include <utility>

namespace saucer
{
    template <typename T>
    class required
    {
        T m_value;

      public:
        template <typename... Ts>
        required(Ts &&...args) : m_value(std::forward<Ts>(args)...)
        {
        }

      public:
        required() = delete;

      public:
        [[nodiscard]] T &value()
        {
            return m_value;
        }

        [[nodiscard]] const T &value() const
        {
            return m_value;
        }

      public:
        [[nodiscard]] T *operator->()
        {
            return std::addressof(m_value);
        }

        [[nodiscard]] const T *operator->() const
        {
            return std::addressof(m_value);
        }
    };
} // namespace saucer
