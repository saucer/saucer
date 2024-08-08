#pragma once

#include <memory>
#include <functional>

#include <gtk/gtk.h>

namespace saucer
{
    template <typename T>
    using custom_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    template <typename T, auto Ref, auto Unref>
    class ref_ptr
    {
        T *m_data;

      private:
        template <auto Action>
        static T *perform(T *);

      public:
        ref_ptr();

      public:
        ref_ptr(T *data);
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
        static ref_ptr copy(T *data);
    };

    template <typename T>
    using g_object_ptr = ref_ptr<T, g_object_ref, g_object_unref>;

    using g_bytes_ptr = ref_ptr<GBytes, g_bytes_ref, g_bytes_unref>;
    using g_event_ptr = ref_ptr<GdkEvent, gdk_event_ref, gdk_event_unref>;
} // namespace saucer

#include "utils.gtk.inl"
