#pragma once
#include <map>
#include <tuple>
#include <vector>
#include <atomic>
#include <lock.hpp>
#include <functional>
#include <type_traits>

namespace saucer
{
    template <auto Val, typename Function> struct event
    {
        static const auto value = Val;
        using callback_t = std::function<Function>;

      private:
        using callback_result_t = typename callback_t::result_type;
        using fire_result_t = std::conditional_t<std::is_same_v<callback_result_t, void>, void, std::vector<callback_result_t>>;

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