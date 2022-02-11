#pragma once
#include <memory>
#include <string>
#include <functional>
#include "events/events.hpp"

namespace saucer
{
    enum class window_event
    {
        resize,
        close
    };

#include "annotations.hpp"
    class window
    {
        struct impl;
        using events = event_handler<                                    //
            event<window_event::resize, void(std::size_t, std::size_t)>, //
            event<window_event::close, bool()>                           //
            >;

      protected:
        events m_events;
        std::unique_ptr<impl> m_impl;

      protected:
        window();

      public:
        virtual ~window();

      public:
        bool get_resizeable() const;
        std::string get_title() const;
        [[thread_safe]] bool get_decorations() const;
        [[thread_safe]] bool get_always_on_top() const;
        [[thread_safe]] std::pair<std::size_t, std::size_t> get_size() const;
        [[thread_safe]] std::pair<std::size_t, std::size_t> get_max_size() const;
        [[thread_safe]] std::pair<std::size_t, std::size_t> get_min_size() const;

      public:
        [[thread_safe]] void hide();
        [[thread_safe]] void show();
        [[thread_safe]] void close();

      public:
        void set_resizeable(bool enabled);
        void set_title(const std::string &);
        [[thread_safe]] void set_decorations(bool enabled);
        [[thread_safe]] void set_always_on_top(bool enabled);
        [[thread_safe]] void set_size(std::size_t width, std::size_t height);
        [[thread_safe]] void set_max_size(std::size_t width, std::size_t height);
        [[thread_safe]] void set_min_size(std::size_t width, std::size_t height);

      public:
        [[thread_safe]] void clear(window_event event);
        [[thread_safe]] void unregister(window_event event, std::size_t id);

        template <window_event Event> //
        [[thread_safe]] std::size_t on(events::get_t<Event> &&) = delete;

      public:
        static void run();
    };
#include "annotations.hpp" //NOLINT

    template <> std::size_t window::on<window_event::close>(events::get_t<window_event::close> &&);
    template <> std::size_t window::on<window_event::resize>(events::get_t<window_event::resize> &&);
} // namespace saucer