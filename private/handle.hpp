#pragma once

namespace saucer::utils
{
    template <typename T, auto Release, T Empty = {}>
    class handle
    {
        T m_handle{Empty};

      public:
        handle();

      public:
        handle(T handle);
        handle(handle &&other) noexcept;

      public:
        ~handle();

      public:
        handle &operator=(handle &&other) noexcept;

      public:
        [[nodiscard]] const T &get() const;

      public:
        T &reset(T handle = Empty);
    };
} // namespace saucer::utils

#include "handle.inl"
