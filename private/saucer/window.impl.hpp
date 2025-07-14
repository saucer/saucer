#pragma once

#include "window.hpp"

namespace saucer
{
    struct window::impl
    {
        struct impl_native;

      public:
        application *parent;
        window::events *events;

      public:
        std::unique_ptr<impl_native> native;

      public:
        impl();

      public:
        ~impl();

      public:
        bool init_native();

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
        [[nodiscard]] bool always_on_top() const;
        [[nodiscard]] bool click_through() const;

      public:
        [[nodiscard]] std::string title() const;
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
        void set_always_on_top(bool);
        void set_click_through(bool);

      public:
        void set_icon(const icon &);
        void set_decorations(decoration);
        void set_title(const std::string &);

      public:
        void set_size(const saucer::size &);
        void set_max_size(const saucer::size &);
        void set_min_size(const saucer::size &);

      public:
        void set_position(const saucer::position &);
    };

    template <typename T, typename Callback, typename... Ts>
        requires std::invocable<Callback, Ts...>
    auto invoke(T *impl, Callback callback, Ts &&...args)
    {
        if (!impl)
        {
            return std::invoke_result_t<Callback, Ts...>{};
        }

        return impl->parent->invoke(callback, std::forward<Ts>(args)...);
    }

    template <typename T, typename Callback, typename... Ts>
        requires std::invocable<Callback, T *, Ts...>
    auto invoke(T *impl, Callback callback, Ts &&...args)
    {
        return invoke(impl, callback, impl, std::forward<Ts>(args)...);
    }
} // namespace saucer
