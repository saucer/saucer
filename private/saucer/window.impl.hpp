#pragma once

#include "window.hpp"

#include <rebind/member.hpp>
#include <rebind/utils/name.hpp>

namespace saucer
{
    struct window::impl
    {
        struct native;

      public:
        application *parent;
        window::events *events;

      public:
        std::unique_ptr<native> platform;

      public:
        impl();

      public:
        ~impl();

      public:
        bool init_platform();

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
        void set_always_on_top(bool);
        void set_click_through(bool);

      public:
        void set_icon(const icon &);
        void set_title(const std::string &);

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

    template <typename T, typename Callback, typename... Ts>
        requires std::invocable<Callback, Ts...>
    auto invoke(T *impl, Callback &&callback, Ts &&...args)
    {
        if (!impl)
        {
            return std::invoke_result_t<Callback, Ts...>{};
        }

        return impl->parent->invoke(std::forward<Callback>(callback), std::forward<Ts>(args)...);
    }

    template <typename T, typename Callback, typename... Ts>
        requires std::invocable<Callback, T *, Ts...>
    auto invoke(T *impl, Callback &&callback, Ts &&...args)
    {
        return invoke(impl, std::forward<Callback>(callback), impl, std::forward<Ts>(args)...);
    }

    template <typename T>
    struct nttp_with_func
    {
        static constexpr std::size_t N = 128;

      public:
        T value;
        char func[N + 1]{};

      public:
        constexpr nttp_with_func(T value, const char *builtin = __builtin_FUNCTION()) : value(value)
        {
            std::copy_n(builtin, std::min(N, std::char_traits<char>::length(builtin)), func);
        }
    };

    template <nttp_with_func Callback, typename T, typename... Ts>
    auto invoke(T *impl, Ts &&...args)
    {
        constexpr auto member = rebind::utils::impl::remove_namespace(rebind::member_name<Callback.value>);
        static_assert(Callback.func == member, "Name of implementation does not match interface!");
        return invoke(impl, Callback.value, impl, std::forward<Ts>(args)...);
    }
} // namespace saucer
