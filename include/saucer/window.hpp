#pragma once

#include "modules/module.hpp"

#include "app.hpp"
#include "icon.hpp"

#include <string>
#include <memory>

#include <cstdint>

#include <ereignis/manager/manager.hpp>

namespace saucer
{
    struct position
    {
        int x;
        int y;
    };

    using size = position;

    struct window
    {
        struct impl;

      public:
        enum class event : std::uint8_t
        {
            decorated,
            maximize,
            minimize,
            closed,
            resize,
            focus,
            close,
        };

      public:
        enum class decoration : std::uint8_t
        {
            none,
            partial,
            full,
        };

      public:
        enum class edge : std::uint8_t
        {
            top          = 1 << 0,
            bottom       = 1 << 1,
            left         = 1 << 2,
            right        = 1 << 3,
            bottom_left  = bottom | left,
            bottom_right = bottom | right,
            top_left     = top | left,
            top_right    = top | right,
        };

      public:
        using events = ereignis::manager<                        //
            ereignis::event<event::decorated, void(decoration)>, //
            ereignis::event<event::maximize, void(bool)>,        //
            ereignis::event<event::minimize, void(bool)>,        //
            ereignis::event<event::closed, void()>,              //
            ereignis::event<event::resize, void(int, int)>,      //
            ereignis::event<event::focus, void(bool)>,           //
            ereignis::event<event::close, policy()>              //
            >;

      protected:
        std::unique_ptr<events> m_events;
        std::unique_ptr<impl> m_impl;

      private:
        window();

      public:
        static std::shared_ptr<window> create(application *);

      public:
        ~window();

      protected:
        template <event Event>
        void setup();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<window, Stable> native() const;

      public:
        [[nodiscard]] application &parent() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool visible() const;
        [[sc::thread_safe]] [[nodiscard]] bool focused() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool minimized() const;
        [[sc::thread_safe]] [[nodiscard]] bool maximized() const;
        [[sc::thread_safe]] [[nodiscard]] bool resizable() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool always_on_top() const;
        [[sc::thread_safe]] [[nodiscard]] bool click_through() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::string title() const;
        [[sc::thread_safe]] [[nodiscard]] decoration decorations() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] saucer::size size() const;
        [[sc::thread_safe]] [[nodiscard]] saucer::size max_size() const;
        [[sc::thread_safe]] [[nodiscard]] saucer::size min_size() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] saucer::position position() const;
        [[sc::thread_safe]] [[nodiscard]] std::optional<saucer::screen> screen() const;

      public:
        [[sc::thread_safe]] void hide();
        [[sc::thread_safe]] void show();
        [[sc::thread_safe]] void close();

      public:
        [[sc::thread_safe]] void focus();

      public:
        [[sc::thread_safe]] void start_drag();
        [[sc::thread_safe]] void start_resize(edge);

      public:
        [[sc::thread_safe]] void set_minimized(bool);
        [[sc::thread_safe]] void set_maximized(bool);
        [[sc::thread_safe]] void set_resizable(bool);

      public:
        [[sc::thread_safe]] void set_always_on_top(bool);
        [[sc::thread_safe]] void set_click_through(bool);

      public:
        [[sc::thread_safe]] void set_icon(const icon &);
        [[sc::thread_safe]] void set_decorations(decoration);
        [[sc::thread_safe]] void set_title(const std::string &);

      public:
        [[sc::thread_safe]] void set_size(const saucer::size &);
        [[sc::thread_safe]] void set_max_size(const saucer::size &);
        [[sc::thread_safe]] void set_min_size(const saucer::size &);

      public:
        [[sc::thread_safe]] void set_position(const saucer::position &);

      public:
        template <event Event>
        [[sc::thread_safe]] auto on(events::event<Event>::listener);

        template <event Event>
        [[sc::thread_safe]] void once(events::event<Event>::listener::callback);

      public:
        template <event Event, typename... Ts>
        [[sc::thread_safe]] auto await(Ts &&...result);

      public:
        [[sc::thread_safe]] void off(event);
        [[sc::thread_safe]] void off(event, std::uint64_t id);
    };
} // namespace saucer

#include "window.inl"
