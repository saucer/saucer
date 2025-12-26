#pragma once

#include <saucer/window.hpp>

namespace saucer
{
    struct window::impl
    {
        struct native;

      public:
        application *parent;
        window::events events;

      public:
        std::unique_ptr<native> platform;

      public:
        impl();

      public:
        ~impl();

      public:
        result<> init_platform();

      public:
        template <event Event>
        void setup();

      public:
        [[nodiscard]] bool visible() const;
        [[nodiscard]] bool focused() const;

      public:
        [[nodiscard]] bool minimized() const;
        [[nodiscard]] bool maximized() const;
        [[nodiscard]] bool resizable() const;

      public:
        [[nodiscard]] bool fullscreen() const;

      public:
        [[nodiscard]] bool always_on_top() const;
        [[nodiscard]] bool click_through() const;

      public:
        [[nodiscard]] std::string title() const;

      public:
        [[nodiscard]] color background() const;
        [[nodiscard]] decoration decorations() const;

      public:
        [[nodiscard]] saucer::size size() const;
        [[nodiscard]] saucer::size max_size() const;
        [[nodiscard]] saucer::size min_size() const;

      public:
        [[nodiscard]] saucer::position position() const;
        [[nodiscard]] std::optional<saucer::screen> screen() const;

      public:
        void hide() const;
        void show() const;
        void close() const;

      public:
        void focus() const;

      public:
        void start_drag() const;
        void start_resize(edge);

      public:
        void set_minimized(bool);
        void set_maximized(bool);
        void set_resizable(bool);

      public:
        void set_fullscreen(bool);

      public:
        void set_always_on_top(bool);
        void set_click_through(bool);

      public:
        void set_icon(const icon &);
        void set_title(cstring_view);

      public:
        void set_background(color);
        void set_decorations(decoration);

      public:
        void set_size(saucer::size);
        void set_max_size(saucer::size);
        void set_min_size(saucer::size);

      public:
        void set_position(saucer::position);
    };
} // namespace saucer
