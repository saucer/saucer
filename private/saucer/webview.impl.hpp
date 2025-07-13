#pragma once

#include "webview.hpp"
#include "window.impl.hpp"

namespace saucer
{
    struct webview::impl
    {
        struct impl_native;

      public:
        webview *self; // TODO: would `on_message` not use inheritance, this could be removed!

      public:
        window::impl *parent;
        webview::events *events;

      public:
        bool attributes;
        embedded_files embedded;

      public:
        std::unique_ptr<impl_native> native;

      public:
        impl(webview *, const options &, window::impl *, webview::events *);

      public:
        ~impl();

      public:
        template <event Event>
        void setup();

      public:
        void handle_scheme(const std::string &, scheme::resolver &&);

      public:
        void reject(std::uint64_t, std::string_view);
        void resolve(std::uint64_t, std::string_view);

      public:
        [[nodiscard]] icon favicon() const;
        [[nodiscard]] std::string page_title() const;

      public:
        [[nodiscard]] bool dev_tools() const;
        [[nodiscard]] bool context_menu() const;
        [[nodiscard]] std::optional<uri> url() const;

      public:
        [[nodiscard]] color background() const;
        [[nodiscard]] bool force_dark_mode() const;

      public:
        void set_dev_tools(bool);
        void set_context_menu(bool);

      public:
        void set_force_dark_mode(bool);
        void set_background(const color &);

      public:
        void set_url(const uri &);

      public:
        void back();
        void forward();

      public:
        void reload();

      public:
        void clear_scripts();

      public:
        void inject(const script &);
        void execute(const std::string &);

      public:
        void remove_scheme(const std::string &);

      public:
        static void register_scheme(const std::string &);
    };

    // TODO: Prevent code duplication here (!)

    template <typename Callback, typename... Ts>
        requires std::invocable<Callback, Ts...>
    auto invoke(webview::impl *impl, Callback callback, Ts &&...args)
    {
        if (!impl)
        {
            return std::invoke_result_t<Callback, Ts...>{};
        }

        return impl->parent->parent->invoke(callback, std::forward<Ts>(args)...);
    }

    template <typename Callback, typename... Ts>
        requires std::invocable<Callback, webview::impl *, Ts...>
    auto invoke(webview::impl *impl, Callback callback, Ts &&...args)
    {
        return invoke(impl, callback, impl, std::forward<Ts>(args)...);
    }
} // namespace saucer
