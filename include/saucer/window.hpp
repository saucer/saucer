#pragma once

#include "icon.hpp"

#include <string>
#include <memory>

#include <set>
#include <utility>
#include <functional>

#include <thread>
#include <cstdint>
#include <filesystem>

#include <ereignis/manager.hpp>

namespace saucer
{
    enum class window_event
    {
        maximize,
        minimize,
        closed,
        resize,
        focus,
        close,
    };

    enum class window_edge
    {
        top    = 1 << 0,
        bottom = 1 << 1,
        left   = 1 << 2,
        right  = 1 << 3,
    };

    struct options
    {
        bool persistent_cookies{true};
        bool hardware_acceleration{true};

      public:
        std::filesystem::path storage_path;
        std::set<std::string> chrome_flags;

      public:
        std::size_t threads = std::thread::hardware_concurrency();
    };

    class window
    {
        struct impl;

      private:
        using callback_t = std::move_only_function<void()>;

      public:
        using events = ereignis::manager<                          //
            ereignis::event<window_event::resize, void(int, int)>, //
            ereignis::event<window_event::maximize, void(bool)>,   //
            ereignis::event<window_event::minimize, void(bool)>,   //
            ereignis::event<window_event::focus, void(bool)>,      //
            ereignis::event<window_event::closed, void()>,         //
            ereignis::event<window_event::close, bool()>           //
            >;

      protected:
        events m_events;
        std::unique_ptr<impl> m_impl;

      protected:
        void dispatch(callback_t callback) const;

      protected:
        window(const options & = {});

      public:
        virtual ~window();

      public:
        [[sc::thread_safe]] [[nodiscard]] bool focused() const;
        [[sc::thread_safe]] [[nodiscard]] bool minimized() const;
        [[sc::thread_safe]] [[nodiscard]] bool maximized() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool resizable() const;
        [[sc::thread_safe]] [[nodiscard]] bool decorations() const;
        [[sc::thread_safe]] [[nodiscard]] bool always_on_top() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::string title() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> max_size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> min_size() const;

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

      public:
        [[sc::thread_safe]] void set_resizable(bool enabled);
        [[sc::thread_safe]] void set_decorations(bool enabled);
        [[sc::thread_safe]] void set_always_on_top(bool enabled);

      public:
        [[sc::thread_safe]] void set_icon(const icon &icon);
        [[sc::thread_safe]] void set_title(const std::string &title);

      public:
        [[sc::thread_safe]] void set_size(int width, int height);
        [[sc::thread_safe]] void set_max_size(int width, int height);
        [[sc::thread_safe]] void set_min_size(int width, int height);

      public:
        [[sc::thread_safe]] void clear(window_event event);
        [[sc::thread_safe]] void remove(window_event event, std::uint64_t id);

        template <window_event Event>
        [[sc::thread_safe]] void once(events::type<Event>);

        template <window_event Event>
        [[sc::thread_safe]] std::uint64_t on(events::type<Event>);

      public:
        template <typename Callback>
        [[sc::thread_safe]] auto dispatch(Callback &&callback) const;

      public:
        template <bool Blocking = true>
        [[sc::may_block]] static void run();
    };
} // namespace saucer

#include "window.inl"
