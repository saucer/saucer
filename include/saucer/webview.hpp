#pragma once

#include "window.hpp"

#include "modules/module.hpp"
#include "utils/required.hpp"

#include "stash/stash.hpp"

#include "uri.hpp"
#include "icon.hpp"
#include "script.hpp"
#include "permission.hpp"

#include "scheme.hpp"
#include "navigation.hpp"

#include <memory>

#include <set>
#include <array>
#include <cstdint>

#include <filesystem>
#include <unordered_map>

#include <string>
#include <string_view>

#include <ereignis/manager/manager.hpp>

namespace saucer
{
    namespace fs = std::filesystem;

    enum class state : std::uint8_t
    {
        started,
        finished,
    };

    enum class status : std::uint8_t
    {
        handled,
        unhandled,
    };

    struct embedded_file
    {
        stash<> content;
        std::string mime;
    };

    struct webview
    {
        struct impl;

      public:
        struct options;

      private:
        using embedded_files = std::unordered_map<fs::path, embedded_file>;

      public:
        enum class event : std::uint8_t
        {
            permission,
            dom_ready,
            navigated,
            navigate,
            message,
            request,
            favicon,
            title,
            load,
        };

      public:
        using events = ereignis::manager<                                                             //
            ereignis::event<event::permission, status(const std::shared_ptr<permission::request> &)>, //
            ereignis::event<event::dom_ready, void()>,                                                //
            ereignis::event<event::navigated, void(const uri &)>,                                     //
            ereignis::event<event::navigate, policy(const navigation &)>,                             //
            ereignis::event<event::message, status(std::string_view)>,                                //
            ereignis::event<event::request, void(const uri &)>,                                       //
            ereignis::event<event::favicon, void(const icon &)>,                                      //
            ereignis::event<event::title, void(std::string_view)>,                                    //
            ereignis::event<event::load, void(const state &)>                                         //
            >;

      protected:
        std::unique_ptr<events> m_events;
        std::unique_ptr<impl> m_impl;

      private:
        webview();

      public:
        webview(webview &&) noexcept;

      public:
        static std::optional<webview> create(const options &);

      public:
        ~webview();

      protected:
        template <event Event>
        void setup();

      protected:
        void handle_scheme(const std::string &, scheme::resolver &&);

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<webview, Stable> native() const;

      public:
        [[nodiscard]] window &parent() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] icon favicon() const;
        [[sc::thread_safe]] [[nodiscard]] std::string page_title() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool dev_tools() const;
        [[sc::thread_safe]] [[nodiscard]] bool context_menu() const;
        [[sc::thread_safe]] [[nodiscard]] std::optional<uri> url() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] color background() const;
        [[sc::thread_safe]] [[nodiscard]] bool force_dark_mode() const;

      public:
        [[sc::thread_safe]] void set_dev_tools(bool);
        [[sc::thread_safe]] void set_context_menu(bool);

      public:
        [[sc::thread_safe]] void set_background(color);
        [[sc::thread_safe]] void set_force_dark_mode(bool);

      public:
        [[sc::thread_safe]] void set_url(const uri &);
        [[sc::thread_safe]] void set_url(const std::string &);

      public:
        [[sc::thread_safe]] void back();
        [[sc::thread_safe]] void forward();

      public:
        [[sc::thread_safe]] void reload();

      public:
        [[sc::thread_safe]] void serve(fs::path);
        [[sc::thread_safe]] void embed(embedded_files);

      public:
        [[sc::thread_safe]] void unembed();
        [[sc::thread_safe]] void unembed(const fs::path &);

      public:
        [[sc::thread_safe]] void execute(const std::string &);
        [[sc::thread_safe]] std::uint64_t inject(const script &);

      public:
        [[sc::thread_safe]] void uninject();
        [[sc::thread_safe]] void uninject(std::uint64_t);

      public:
        template <typename T>
        [[sc::thread_safe]] void handle_scheme(const std::string &name, T &&handler);
        [[sc::thread_safe]] void remove_scheme(const std::string &name);

      public:
        template <event Event>
        [[sc::thread_safe]] auto on(events::event<Event>::listener);

        template <event Event>
        [[sc::thread_safe]] void once(events::event<Event>::listener::callback);

      public:
        template <event Event, typename... Ts>
        [[sc::thread_safe]] auto await(Ts &&...result);

      public:
        [[sc::thread_safe]] void off(event);
        [[sc::thread_safe]] void off(event, std::uint64_t id);

      public:
        [[sc::before_init]] static void register_scheme(const std::string &name);
    };

    struct webview::options
    {
        required<std::shared_ptr<saucer::window>> window;

      public:
        bool attributes{true};
        bool persistent_cookies{true};
        bool hardware_acceleration{true};

      public:
        std::optional<fs::path> storage_path;
        std::optional<std::string> user_agent;

      public:
        std::set<std::string> browser_flags;
    };
} // namespace saucer

#include "webview.inl"
