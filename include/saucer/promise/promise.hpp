#pragma once
#include "../serializers/serializer.hpp"

#include <future>
#include <functional>
#include <lockpp/lock.hpp>

namespace saucer
{
    class promise_base
    {
        using fail_callback_t = std::function<void(const serializer::error &)>;

      protected:
        bool is_safe() const;
        std::thread::id m_creation_thread;
        lockpp::lock<fail_callback_t, std::recursive_mutex> m_fail_callback;

      public:
        virtual ~promise_base() = default;
        promise_base(const std::thread::id &);

      public:
        virtual void wait() = 0;
        virtual bool is_ready() = 0;
        virtual void reject(const serializer::error &);
        virtual promise_base &fail(const fail_callback_t &);
    };

    template <typename T> class promise;
    template <typename... T> static auto all(const std::shared_ptr<promise<T>> &...);

    template <typename T> class promise : public promise_base
    {
        template <typename... O> friend auto all(const std::shared_ptr<promise<O>> &...);
        using callback_t = std::function<void(const T &)>;

      protected:
        std::promise<T> m_promise;
        std::shared_future<T> m_future;
        lockpp::lock<callback_t> m_callback;

      public:
        T get();
        void wait() override;
        void resolve(const T &);
        bool is_ready() override;
        promise<T> &then(const callback_t &);
        void reject(const serializer::error &) override;

      public:
        ~promise() override;
        promise(const std::thread::id &);
    };

    template <> class promise<void> : public promise_base
    {
        template <typename... O> friend auto all(const std::shared_ptr<promise<O>> &...);
        using callback_t = std::function<void()>;

      protected:
        std::promise<void> m_promise;
        std::shared_future<void> m_future;
        lockpp::lock<callback_t> m_callback;

      public:
        void get();
        void resolve();
        void wait() override;
        bool is_ready() override;
        promise<void> &then(const callback_t &);
        void reject(const serializer::error &) override;

      public:
        ~promise() override;
        promise(const std::thread::id &);
    };
} // namespace saucer

#include "promise.inl"