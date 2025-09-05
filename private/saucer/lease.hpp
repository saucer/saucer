#pragma once

#include <memory>
#include <shared_mutex>

namespace saucer::utils
{
    template <typename T>
    class rental;

    template <typename T>
    class lease
    {
        friend class rental<T>;
        struct state;

      private:
        std::shared_ptr<state> m_state;

      public:
        lease();

      public:
        explicit lease(T);
        lease(lease &&) noexcept;

      public:
        lease &operator=(lease &&) noexcept;

      public:
        ~lease();

      public:
        [[nodiscard]] T &value() &;
        [[nodsicard]] rental<T> rent() const;
    };

    template <typename T>
    class rental
    {
        class lock;
        using state = lease<T>::state;

      private:
        std::shared_ptr<state> m_state;

      public:
        rental();

      public:
        explicit rental(const lease<T> &);

      public:
        rental(const rental &);
        rental(rental &&) noexcept;

      public:
        [[nodiscard]] lock access() const;
    };

    template <typename T>
    class rental<T>::lock
    {
        std::shared_ptr<state> m_state;
        std::shared_lock<std::shared_mutex> m_guard;

      public:
        explicit lock(const rental &);

      public:
        [[nodiscard]] T *value() const;
    };

    template <typename T, typename Func>
    auto defer(lease<T> &, Func &&);
} // namespace saucer::utils

#include "lease.inl"
