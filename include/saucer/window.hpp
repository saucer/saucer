#pragma once
#include "utils/color.hpp"

#include <memory>
#include <string>
#include <filesystem>
#include <ereignis/manager.hpp>

namespace saucer
{
    enum class window_event
    {
        closed,
        resize,
        close,
    };

    struct options
    {
        bool persistent_cookies{true};
        bool hardware_acceleration{true};
        std::filesystem::path storage_path;
    };

#include "meta/annotations.hpp"
    class window
    {
        struct impl;

      private:
        using events = ereignis::event_manager<                    //
            ereignis::event<window_event::resize, void(int, int)>, //
            ereignis::event<window_event::closed, void()>,         //
            ereignis::event<window_event::close, bool()>           //
            >;

      protected:
        events m_events;
        std::unique_ptr<impl> m_impl;

      protected:
        window(const options & = {});

      public:
        virtual ~window();

      public:
        [[thread_safe]] [[nodiscard]] bool resizable() const;
        [[thread_safe]] [[nodiscard]] bool decorations() const;
        [[thread_safe]] [[nodiscard]] bool always_on_top() const;

      public:
        [[thread_safe]] [[nodiscard]] std::string title() const;
        [[thread_safe]] [[nodiscard]] color background() const;

      public:
        [[thread_safe]] [[nodiscard]] std::pair<int, int> size() const;
        [[thread_safe]] [[nodiscard]] std::pair<int, int> max_size() const;
        [[thread_safe]] [[nodiscard]] std::pair<int, int> min_size() const;

      public:
        [[thread_safe]] void hide();
        [[thread_safe]] void show();
        [[thread_safe]] void close();

      public:
        [[thread_safe]] void set_resizable(bool enabled);
        [[thread_safe]] void set_decorations(bool enabled);
        [[thread_safe]] void set_always_on_top(bool enabled);

      public:
        [[thread_safe]] void set_title(const std::string &);
        [[thread_safe]] void set_background(const color &color);

      public:
        [[thread_safe]] void set_size(int width, int height);
        [[thread_safe]] void set_max_size(int width, int height);
        [[thread_safe]] void set_min_size(int width, int height);

      public:
        [[thread_safe]] void clear(window_event event);
        [[thread_safe]] void remove(window_event event, std::uint64_t id);

        template <window_event Event> //
        [[thread_safe]] std::uint64_t on(events::callback_t<Event> &&) = delete;

      public:
        template <bool Blocking = true> static void run();
    };
#include "meta/annotations.hpp" //NOLINT

    template <> std::uint64_t window::on<window_event::close>(events::callback_t<window_event::close> &&);
    template <> std::uint64_t window::on<window_event::closed>(events::callback_t<window_event::closed> &&);
    template <> std::uint64_t window::on<window_event::resize>(events::callback_t<window_event::resize> &&);
} // namespace saucer