#pragma once

namespace saucer::utils
{
    template <typename T, auto Ref, auto Unref, T Empty = T{}>
    class ref_obj
    {
        T m_value{Empty};

      private:
        template <auto Action>
        static T perform(T);

      public:
        ref_obj();

      public:
        ref_obj(T valeu);
        ref_obj(const ref_obj &other);
        ref_obj(ref_obj &&other) noexcept;

      public:
        ~ref_obj();

      public:
        ref_obj &operator=(const ref_obj &other);
        ref_obj &operator=(ref_obj &&other) noexcept;

      public:
        [[nodiscard]] T get() const;
        [[nodiscard]] explicit operator bool() const;

      public:
        void reset(T value = nullptr);

      public:
        static ref_obj ref(T);
    };

    template <typename T, auto Ref, auto Unref>
    using ref_ptr = ref_obj<T *, Ref, Unref, nullptr>;
} // namespace saucer::utils

#include "ref_obj.inl"
