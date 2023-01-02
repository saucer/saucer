#pragma once
#include <array>
#include <memory>
#include <string>
#include <functional>
#include <ereignis/manager.hpp>

namespace saucer
{
    enum class window_event
    {
        closed,
        resize,
        close,
    };

#include "annotations.hpp"
    class window
    {
        struct impl;

      private:
        using events = ereignis::event_manager<                                    //
            ereignis::event<window_event::resize, void(std::size_t, std::size_t)>, //
            ereignis::event<window_event::closed, void()>,                         //
            ereignis::event<window_event::close, bool()>                           //
            >;

      protected:
        events m_events;
        std::unique_ptr<impl> m_impl;

      protected:
        window();

      public:
        virtual ~window();

      public:
        [[thread_safe]] [[nodiscard]] bool get_resizable() const;
        [[thread_safe]] [[nodiscard]] bool get_decorations() const;
        [[thread_safe]] [[nodiscard]] std::string get_title() const;
        [[thread_safe]] [[nodiscard]] bool get_always_on_top() const;
        [[thread_safe]] [[nodiscard]] std::pair<std::size_t, std::size_t> get_size() const;
        [[thread_safe]] [[nodiscard]] std::pair<std::size_t, std::size_t> get_max_size() const;
        [[thread_safe]] [[nodiscard]] std::pair<std::size_t, std::size_t> get_min_size() const;
        [[thread_safe]] [[nodiscard]] std::array<std::size_t, 4> get_background_color() const;

      public:
        [[thread_safe]] void hide();
        [[thread_safe]] void show();
        [[thread_safe]] void close();

      public:
        [[thread_safe]] void set_resizable(bool enabled);
        [[thread_safe]] void set_decorations(bool enabled);
        [[thread_safe]] void set_title(const std::string &);
        [[thread_safe]] void set_always_on_top(bool enabled);
        [[thread_safe]] void set_size(std::size_t width, std::size_t height);
        [[thread_safe]] void set_max_size(std::size_t width, std::size_t height);
        [[thread_safe]] void set_min_size(std::size_t width, std::size_t height);
        [[thread_safe]] void set_background_color(std::size_t r, std::size_t g, std::size_t b, std::size_t a);

      public:
        [[thread_safe]] void clear(window_event event);
        [[thread_safe]] void remove(window_event event, std::uint64_t id);

        template <window_event Event> //
        [[thread_safe]] std::uint64_t on(events::callback_t<Event> &&) = delete;

      public:
        template <bool Blocking = true> static void run();
    };
#include "annotations.hpp" //NOLINT

    template <> std::uint64_t window::on<window_event::close>(events::callback_t<window_event::close> &&);
    template <> std::uint64_t window::on<window_event::closed>(events::callback_t<window_event::closed> &&);
    template <> std::uint64_t window::on<window_event::resize>(events::callback_t<window_event::resize> &&);
} // namespace saucer