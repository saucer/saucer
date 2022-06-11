#pragma once
#include <map>
#include <tuple>
#include <atomic>
#include <functional>
#include <type_traits>
#include <lockpp/lock.hpp>

namespace saucer
{
    template <typename Result, typename Iterator, typename... Params> class callback_iterator
    {
      private:
        Iterator m_iterator;
        std::tuple<Params...> &m_params;

      public:
        callback_iterator(Iterator &&, std::tuple<Params...> &);

      public:
        Result operator*();
        callback_iterator operator++();
        bool operator!=(const callback_iterator &);
    };

    template <typename Type, typename... Params> class callback_invoker
    {
        using callback_t = std::function<Type>;
        using callbacks_t = std::map<std::size_t, callback_t>;
        using raw_iterator_t = typename callbacks_t::iterator;
        using callback_result_t = typename callback_t::result_type;

      private:
        callbacks_t m_callbacks;
        std::tuple<Params...> m_params;

      public:
        callback_invoker(callbacks_t &&, std::tuple<Params...> &&);

      public:
        auto begin();
        auto end();
    };

    template <auto Val, typename Type> struct event
    {
        static const auto value = Val;
        using callback_t = std::function<Type>;

      private:
        using callback_result_t = typename callback_t::result_type;
        using fire_result_t = std::conditional_t<std::is_same_v<callback_result_t, void>, void, callback_invoker<Type>>;

      private:
        std::atomic<std::size_t> m_counter{0};
        lockpp::lock<std::map<std::size_t, callback_t>> m_callbacks;

      public:
        void clear_callbacks();
        std::size_t add_callback(callback_t &&);
        void remove_callback(const std::size_t &);
        template <typename... Params> fire_result_t fire(Params &&...);
    };

    template <typename> struct is_event : std::false_type
    {
    };
    template <auto Val, typename Function> struct is_event<event<Val, Function>> : std::true_type
    {
    };

    template <typename T> struct type_identity
    {
        using type = T;
    };

    template <typename... Events> struct event_handler
    {
        static_assert((is_event<Events>::value && ...));
        using tuple_t = std::tuple<Events...>;

      public:
        template <auto Val, std::size_t I = 0> static constexpr auto get();
        template <auto Val> using get_t = typename decltype(get<Val>())::type;

      public:
        tuple_t events;
        template <auto Val, std::size_t I = 0> constexpr auto &at();
        template <typename EventType, std::size_t I = 0> constexpr void clear(EventType);
        template <typename EventType, std::size_t I = 0> constexpr void unregister(EventType, std::size_t);
    };
} // namespace saucer

#include "events.inl"