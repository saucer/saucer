#pragma once
#include "promise.hpp"
#include <exception>

#ifdef THROW_ASSERT
#define saucer_assert(expr)                                                                                                                                                                  \
    if (!expr)                                                                                                                                                                               \
        throw std::runtime_error(#expr);
#else
#include <cassert>
#define saucer_assert assert
#endif

namespace saucer
{
    inline promise_base::promise_base(const std::thread::id &creation_thread) : m_creation_thread(creation_thread) {}
    inline bool promise_base::is_safe() const
    {
        return !(m_creation_thread == std::this_thread::get_id());
    }

    inline promise_base &promise_base::fail(const fail_callback_t &callback)
    {
        m_fail_callback.assign(callback);
        return *this;
    }

    inline void promise_base::reject(const serializer::error &error)
    {
        if (*m_fail_callback.read())
        {
            (*m_fail_callback.read())(error);
        }
    }

    template <typename T> promise<T>::~promise() = default;
    template <typename T> promise<T>::promise(const std::thread::id &creation_thread) : promise_base(creation_thread), m_future(m_promise.get_future()) {}

    template <typename T> void promise<T>::resolve(const T &value)
    {
        m_promise.set_value(value);

        if (*m_callback.read())
        {
            (*m_callback.read())(value);
        }
    }

    template <typename T> bool promise<T>::is_ready()
    {
        return m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
    }

    template <typename T> void promise<T>::wait()
    {
        saucer_assert((void("Can't wait on promise in main thread"), is_safe()));
        m_future.wait();
    }

    template <typename T> T promise<T>::get()
    {
        saucer_assert((void("get() should not be called by the main thread when the result is not ready"), is_safe() ? true : is_ready()));
        return m_future.get();
    }

    template <typename T> promise<T> &promise<T>::then(const callback_t &callback)
    {
        m_callback.assign(callback);
        return *this;
    }

    template <typename T> void promise<T>::reject(const serializer::error &error)
    {
        m_promise.set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)));
        promise_base::reject(error);
    }

    inline promise<void>::~promise() = default;
    inline promise<void>::promise(const std::thread::id &creation_thread) : promise_base(creation_thread), m_future(m_promise.get_future()) {}

    inline void promise<void>::resolve()
    {
        m_promise.set_value();

        if (*m_callback.read())
        {
            (*m_callback.read())();
        }
    }

    inline bool promise<void>::is_ready()
    {
        return m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
    }

    inline void promise<void>::wait()
    {
        saucer_assert((void("Can't wait on promise in main thread"), is_safe()));
        m_future.wait();
    }

    inline void promise<void>::get()
    {
        saucer_assert((void("get() should not be called by the main thread when the result is not ready"), is_safe() ? true : is_ready()));
        m_future.get();
    }

    inline promise<void> &promise<void>::then(const callback_t &callback)
    {
        m_callback.assign(callback);
        return *this;
    }

    inline void promise<void>::reject(const serializer::error &error)
    {
        m_promise.set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)));
        promise_base::reject(error);
    }

    template <typename... T> auto all(const std::shared_ptr<promise<T>> &...promises)
    {
        using tuple_t = decltype(std::tuple_cat(std::tuple<>(), std::conditional_t<std::is_same_v<T, void>, std::tuple<>, std::tuple<T>>()...));
        auto rtn = std::make_shared<promise<tuple_t>>(std::get<0>(std::make_tuple(promises...))->m_creation_thread);
        auto counter = std::make_shared<std::atomic<std::size_t>>();

        auto callback = [=](const auto &...) {
            (*counter)++;
            if (counter->load() == sizeof...(promises))
            {
                auto get_result = [](const auto &item) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(item)>, std::shared_ptr<promise<void>>>)
                    {
                        return std::tuple<>();
                    }
                    else
                    {
                        return std::make_tuple(item->m_future.get());
                    }
                };

                rtn->resolve(std::tuple_cat(std::tuple<>(), get_result(promises)...));
            }
        };

        auto fail_callback = [=]() {
            rtn->reject();
            (promises->then(nullptr).fail(nullptr), ...);
        };

        (promises->then(callback).fail(fail_callback), ...);
        return rtn;
    }
} // namespace saucer