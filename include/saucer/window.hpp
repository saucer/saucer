#pragma once
#include <array>
#include <memory>
#include <string>
#include <functional>
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

#include "annotations.hpp"
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
        [[thread_safe]] [[nodiscard]] bool get_resizable() const;
        [[thread_safe]] [[nodiscard]] bool get_decorations() const;
        [[thread_safe]] [[nodiscard]] std::string get_title() const;
        [[thread_safe]] [[nodiscard]] bool get_always_on_top() const;
        [[thread_safe]] [[nodiscard]] std::array<int, 2> get_size() const;
        [[thread_safe]] [[nodiscard]] std::array<int, 2> get_max_size() const;
        [[thread_safe]] [[nodiscard]] std::array<int, 2> get_min_size() const;
        [[thread_safe]] [[nodiscard]] std::array<int, 4> get_background_color() const;

      public:
        [[thread_safe]] void hide();
        [[thread_safe]] void show();
        [[thread_safe]] void close();

      public:
        [[thread_safe]] void set_resizable(bool enabled);
        [[thread_safe]] void set_decorations(bool enabled);
        [[thread_safe]] void set_title(const std::string &);
        [[thread_safe]] void set_always_on_top(bool enabled);
        [[thread_safe]] void set_size(int width, int height);
        [[thread_safe]] void set_max_size(int width, int height);
        [[thread_safe]] void set_min_size(int width, int height);
        [[thread_safe]] void set_background_color(int r, int g, int b, int a);

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