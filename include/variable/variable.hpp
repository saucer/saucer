#pragma once
#include <type_traits>

#include "../events/events.hpp"

namespace saucer
{
    enum class variable_event
    {
        updated
    };

    template <typename> class proxy;
    template <typename T> class variable
    {
        friend class proxy<T>;
        using events = event_handler<              //
            event<variable_event::updated, void()> //
            >;

      private:
        T m_value;

      protected:
        events m_events;

      public:
        template <typename... Args> explicit variable(Args &&...);

      public:
        template <typename T_ = T, typename = std::enable_if_t<std::is_move_assignable_v<T_>>> void assign(std::decay_t<T> &&);
        template <typename T_ = T, typename = std::enable_if_t<std::is_copy_assignable_v<T_>>> void assign(const std::decay_t<T> &);

      public:
        [[nodiscard]] proxy<T> modify();
        [[nodiscard]] const T &read() const;

      public:
        void clear(variable_event event);
        void unregister(variable_event event, std::size_t id);

        template <variable_event Event> //
        std::size_t on(typename events::template get_t<Event> &&callback);
    };

    template <typename T> class proxy
    {
        friend class variable<T>;

      private:
        T &m_value;
        variable<T> &m_parent;

      protected:
        proxy(T &, variable<T> &);

      public:
        virtual ~proxy();

      public:
        [[nodiscard]] std::add_lvalue_reference_t<T> operator*() noexcept;
        [[nodiscard]] std::add_lvalue_reference_t<T> operator*() const noexcept;

      public:
        [[nodiscard]] std::add_pointer_t<T> operator->() noexcept;
        [[nodiscard]] std::add_pointer_t<T> operator->() const noexcept;
    };
} // namespace saucer

#include "variable.inl"