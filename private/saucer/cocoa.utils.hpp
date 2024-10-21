#pragma once

namespace saucer
{
    template <typename T>
    class objc_ptr
    {
        T *m_data;

      private:
        static T *retain(T *);
        static T *release(T *);

      public:
        objc_ptr();

      public:
        objc_ptr(T *data);
        objc_ptr(const objc_ptr &other);
        objc_ptr(objc_ptr &&other) noexcept;

      public:
        ~objc_ptr();

      public:
        objc_ptr &operator=(const objc_ptr &other);
        objc_ptr &operator=(objc_ptr &&other) noexcept;

      public:
        [[nodiscard]] T *get() const;
        [[nodiscard]] explicit operator bool() const;

      public:
        void reset(T *other = nullptr);

      public:
        static objc_ptr ref(T *data);
    };

    class [[maybe_unused]] autorelease_guard
    {
        void *m_pool;

      public:
        autorelease_guard();

      public:
        ~autorelease_guard();
    };
} // namespace saucer

#include "cocoa.utils.inl"
