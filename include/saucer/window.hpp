#pragma once
#include <memory>
#include <string>
#include <filesystem>
#include <ereignis/manager.hpp>

namespace saucer
{
    enum class window_event : std::uint8_t
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

    using color = std::array<std::uint8_t, 4>;

    class window
    {
        struct impl;

      private:
        using events = ereignis::manager<                          //
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
        [[sc::thread_safe]] [[nodiscard]] bool resizable() const;
        [[sc::thread_safe]] [[nodiscard]] bool decorations() const;
        [[sc::thread_safe]] [[nodiscard]] bool always_on_top() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::string title() const;
        [[sc::thread_safe]] [[nodiscard]] color background() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> max_size() const;
        [[sc::thread_safe]] [[nodiscard]] std::pair<int, int> min_size() const;

      public:
        [[sc::thread_safe]] void hide();
        [[sc::thread_safe]] void show();
        [[sc::thread_safe]] void close();

      public:
        [[sc::thread_safe]] void set_resizable(bool enabled);
        [[sc::thread_safe]] void set_decorations(bool enabled);
        [[sc::thread_safe]] void set_always_on_top(bool enabled);

      public:
        [[sc::thread_safe]] void set_title(const std::string &);
        [[sc::thread_safe]] void set_background(const color &color);

      public:
        [[sc::thread_safe]] void set_size(int width, int height);
        [[sc::thread_safe]] void set_max_size(int width, int height);
        [[sc::thread_safe]] void set_min_size(int width, int height);

      public:
        [[sc::thread_safe]] void clear(window_event event);
        [[sc::thread_safe]] void remove(window_event event, std::uint64_t id);

        template <window_event Event>
        [[sc::thread_safe]] std::uint64_t on(events::type_t<Event> &&) = delete;

      public:
        template <bool Blocking = true>
        static void run();
    };

    template <>
    std::uint64_t window::on<window_event::close>(events::type_t<window_event::close> &&);
    template <>
    std::uint64_t window::on<window_event::closed>(events::type_t<window_event::closed> &&);
    template <>
    std::uint64_t window::on<window_event::resize>(events::type_t<window_event::resize> &&);
} // namespace saucer
