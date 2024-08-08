#pragma once

#include <utility>

#include <memory>
#include <functional>

#include <gtk/gtk.h>

namespace saucer
{
    template <typename T>
    using custom_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    template <typename T, auto ref = g_object_ref, auto unref = g_object_unref>
    class object_ptr
    {
        T *m_data;

      private:
        auto copy_ref(T *data)
        {
            if (data)
            {
                ref(data);
            }

            return data;
        }

      public:
        object_ptr() = default;

      public:
        explicit object_ptr(T *data) : m_data(data) {}
        object_ptr(const object_ptr &other) : m_data(copy_ref(other.m_data)) {}
        object_ptr(object_ptr &&other) noexcept : m_data(std::exchange(other.m_data, nullptr)) {}

      public:
        object_ptr &operator=(const object_ptr &other)
        {
            if (this != &other)
            {
                m_data = copy_ref(other.m_data);
            }

            return *this;
        }

        object_ptr &operator=(object_ptr &&other) noexcept
        {
            if (this != &other)
            {
                m_data = std::exchange(other.m_data, nullptr);
            }

            return *this;
        }

      public:
        ~object_ptr()
        {
            if (!m_data)
            {
                return;
            }

            unref(m_data);
        }

      public:
        T *get() const
        {
            return m_data;
        }

        explicit operator bool() const
        {
            return m_data != nullptr;
        }

      public:
        static object_ptr copy(T *data)
        {
            ref(data);
            return object_ptr{data};
        }
    };

    using bytes_ptr = object_ptr<GBytes, g_bytes_ref, g_bytes_unref>;
    using event_ptr = object_ptr<GdkEvent, gdk_event_ref, gdk_event_unref>;
} // namespace saucer
