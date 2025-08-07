#pragma once

#include "webview.hpp"

namespace saucer
{
    struct webview::impl
    {
        struct native;

      public:
        std::shared_ptr<saucer::window> window;

      public:
        application *parent;
        webview::events *events;

      public:
        bool attributes;
        embedded_files embedded;

      public:
        std::unique_ptr<native> platform;

      public:
        impl();

      public:
        ~impl();

      public:
        result<> init_platform(const options &);

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
        void set_background(color);
        void set_force_dark_mode(bool);

      public:
        void set_url(const uri &);

      public:
        void back();
        void forward();

      public:
        void reload();

      public:
        void execute(const std::string &);
        std::uint64_t inject(const script &);

      public:
        void uninject();
        void uninject(std::uint64_t);

      public:
        void remove_scheme(const std::string &);
        static void register_scheme(const std::string &);

      public:
        status on_message(std::string_view);

      public:
        static std::string ready_script();
        static std::string creation_script();
        static std::string attribute_script();
    };
} // namespace saucer
