#pragma once

namespace saucer
{
    template <typename T>
    class required
    {
        T m_value;

      public:
        template <typename... Ts>
        required(Ts &&...);

      public:
        required() = delete;

      public:
        [[nodiscard]] T &value();
        [[nodiscard]] const T &value() const;

      public:
        [[nodiscard]] T *operator->();
        [[nodiscard]] const T *operator->() const;
    };
} // namespace saucer

#include "required.inl"
