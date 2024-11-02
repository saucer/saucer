#pragma once

namespace saucer::utils
{
    template <typename T, auto Ref, auto Unref>
    class ref_ptr
    {
        T *m_ptr{nullptr};

      private:
        template <auto Action>
        static T *perform(T *);

      public:
        ref_ptr();

      public:
        ref_ptr(T *ptr);
        ref_ptr(const ref_ptr &other);
        ref_ptr(ref_ptr &&other) noexcept;

      public:
        ~ref_ptr();

      public:
        ref_ptr &operator=(const ref_ptr &other);
        ref_ptr &operator=(ref_ptr &&other) noexcept;

      public:
        [[nodiscard]] T *get() const;
        [[nodiscard]] explicit operator bool() const;

      public:
        void reset(T *ptr = nullptr);

      public:
        static ref_ptr ref(T *);
    };
} // namespace saucer::utils

#include "ref_ptr.inl"
