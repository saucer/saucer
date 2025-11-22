#pragma once

#include <saucer/webview.hpp>

namespace saucer
{
    struct webview::impl
    {
        struct native;

      public:
        std::shared_ptr<saucer::window> window;

      public:
        application *parent;
        webview::events events;

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
        void handle(const std::string &, scheme::resolver &&);

      public:
        void reject(std::size_t, std::string_view);
        void resolve(std::size_t, std::string_view);

      public:
        [[nodiscard]] result<saucer::url> url() const;

      public:
        [[nodiscard]] icon favicon() const;
        [[nodiscard]] std::string page_title() const;

      public:
        [[nodiscard]] bool dev_tools() const;
        [[nodiscard]] bool context_menu() const;

      public:
        [[nodiscard]] bool force_dark() const;
        [[nodiscard]] color background() const;

      public:
        [[nodiscard]] saucer::bounds bounds() const;

      public:
        void set_url(const saucer::url &);
        void set_html(const std::string &);

      public:
        void set_dev_tools(bool);
        void set_context_menu(bool);

      public:
        void set_force_dark(bool);
        void set_background(color);

      public:
        void reset_bounds();
        void set_bounds(saucer::bounds);

      public:
        void back();
        void forward();

      public:
        void reload();

      public:
        void execute(const std::string &);
        std::size_t inject(const script &);

      public:
        void uninject();
        void uninject(std::size_t);

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
