#pragma once
#include <functional>
#include <lock.hpp>

namespace saucer
{
    class base_promise
    {
      public:
        virtual ~base_promise() = default;
    };

    template <typename T> class promise : base_promise
    {
        using callback_t = std::function<void(const T &)>;

      protected:
        lockpp::lock<callback_t> m_callback;

      public:
        void resolve(const T &);
        void then(const callback_t &);

      public:
        ~promise() override;
    };

    template <> class promise<void> : base_promise
    {
        using callback_t = std::function<void()>;

      protected:
        lockpp::lock<callback_t> m_callback;

      public:
        void resolve();
        void then(const callback_t &);

      public:
        ~promise() override;
    };
} // namespace saucer

#include "promise.inl"