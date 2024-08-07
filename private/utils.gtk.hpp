#pragma once

#include <utility>

#include <gtk/gtk.h>

namespace saucer
{
    template <typename T, auto ref = g_object_ref, auto unref = g_object_unref>
    class object_ptr
    {
        T *m_data;

      public:
        object_ptr() = default;

      public:
        explicit object_ptr(T *data) : m_data(data) {}

      public:
        object_ptr(const object_ptr &other) : m_data(other.m_data)
        {
            if (!m_data)
            {
                return;
            }

            ref(m_data);
        }

        object_ptr(object_ptr &&other) noexcept : m_data(std::exchange(other.m_data, nullptr)) {}

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
        static object_ptr copy(T *data)
        {
            ref(data);
            return object_ptr{data};
        }
    };
} // namespace saucer
