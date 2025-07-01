#pragma once

#include "modules/module.hpp"

#include "app.hpp"
#include "icon.hpp"

#include <string>
#include <utility>

#include <memory>
#include <cstdint>

#include <ereignis/manager/manager.hpp>

namespace saucer
{
    enum class window_event : std::uint8_t
    {
        decorated,
        maximize,
        minimize,
        closed,
        resize,
        focus,
        close,
    };

    enum class window_decoration : std::uint8_t
    {
        none,
        partial,
        full,
    };

    enum class window_edge : std::uint8_t
    {
        top    = 1 << 0,
        bottom = 1 << 1,
        left   = 1 << 2,
        right  = 1 << 3,
    };

    enum class policy : std::uint8_t
    {
        allow,
        block,
    };

    struct window
    {
        struct impl;

      public:
        using events = ereignis::manager<                                      //
            ereignis::event<window_event::decorated, void(window_decoration)>, //
            ereignis::event<window_event::maximize, void(bool)>,               //
            ereignis::event<window_event::minimize, void(bool)>,               //
            ereignis::event<window_event::closed, void()>,                     //
            ereignis::event<window_event::resize, void(int, int)>,             //
            ereignis::event<window_event::focus, void(bool)>,                  //
            ereignis::event<window_event::close, policy()>                     //
            >;

      protected:
        events m_events;

      protected:
        application *m_parent;
        std::unique_ptr<impl> m_impl;

      protected:
        window(saucer::application *);

      public:
        virtual ~window();

      protected:
        template <window_event Event>
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
        [[sc::thread_safe]] [[nodiscard]] window_decoration decoration() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> max_size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> min_size() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> position() const;
        [[sc::thread_safe]] [[nodiscard]] std::optional<saucer::screen> screen() const;

      public:
        [[sc::thread_safe]] void hide();
        [[sc::thread_safe]] void show();
        [[sc::thread_safe]] void close();

      public:
        [[sc::thread_safe]] void focus();

      public:
        [[sc::thread_safe]] void start_drag();
        [[sc::thread_safe]] void start_resize(window_edge edge);

      public:
        [[sc::thread_safe]] void set_minimized(bool enabled);
        [[sc::thread_safe]] void set_maximized(bool enabled);
        [[sc::thread_safe]] void set_resizable(bool enabled);

      public:
        [[sc::thread_safe]] void set_always_on_top(bool enabled);
        [[sc::thread_safe]] void set_click_through(bool enabled);

      public:
        [[sc::thread_safe]] void set_icon(const icon &icon);
        [[sc::thread_safe]] void set_title(const std::string &title);
        [[sc::thread_safe]] void set_decoration(window_decoration decoration);

      public:
        [[sc::thread_safe]] void set_size(int width, int height);
        [[sc::thread_safe]] void set_max_size(int width, int height);
        [[sc::thread_safe]] void set_min_size(int width, int height);

      public:
        [[sc::thread_safe]] void set_position(int x, int y);

      public:
        [[sc::thread_safe]] void clear(window_event event);
        [[sc::thread_safe]] void remove(window_event event, std::uint64_t id);

        template <window_event Event, typename T>
        [[sc::thread_safe]] void once(T &&callback);

        template <window_event Event, typename T>
        [[sc::thread_safe]] auto on(T &&callback);

        template <window_event Event, typename... Ts>
        [[sc::thread_safe]] auto await(Ts &&...result);
    };
} // namespace saucer

#include "window.inl"
