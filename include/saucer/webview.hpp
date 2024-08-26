#pragma once

#include "icon.hpp"
#include "window.hpp"
#include "scheme.hpp"

#include <filesystem>
#include <unordered_map>

#include <string>
#include <memory>

#include <lockpp/lock.hpp>
#include <ereignis/manager.hpp>

namespace saucer
{
    namespace fs = std::filesystem;

    enum class load_time
    {
        creation,
        ready,
    };

    enum class web_frame
    {
        top,
        all,
    };

    enum class web_event
    {
        title_changed,
        load_finished,
        icon_changed,
        load_started,
        url_changed,
        dom_ready,
    };

    struct embedded_file
    {
        stash<> content;
        std::string mime;
    };

    using color = std::array<std::uint8_t, 4>;

    class webview : public window
    {
        struct impl;

      private:
        using embedded_files = std::unordered_map<std::string, embedded_file>;

      public:
        using events = ereignis::manager<                                         //
            ereignis::event<web_event::title_changed, void(const std::string &)>, //
            ereignis::event<web_event::load_finished, void()>,                    //
            ereignis::event<web_event::icon_changed, void(const icon &)>,         //
            ereignis::event<web_event::load_started, void()>,                     //
            ereignis::event<web_event::url_changed, void(const std::string &)>,   //
            ereignis::event<web_event::dom_ready, void()>                         //
            >;

      private:
        events m_events;
        lockpp::lock<embedded_files> m_embedded_files;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        virtual bool on_message(const std::string &);

      public:
        webview(const options & = {});

      public:
        ~webview() override;

      public:
        [[sc::thread_safe]] [[nodiscard]] icon favicon() const;
        [[sc::thread_safe]] [[nodiscard]] std::string page_title() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool dev_tools() const;
        [[sc::thread_safe]] [[nodiscard]] std::string url() const;
        [[sc::thread_safe]] [[nodiscard]] bool context_menu() const;

      public:
        [[sc::thread_safe]] [[nodiscard]] color background() const;
        [[sc::thread_safe]] [[nodiscard]] bool force_dark_mode() const;

      public:
        [[sc::thread_safe]] void set_dev_tools(bool enabled);
        [[sc::thread_safe]] void set_context_menu(bool enabled);

      public:
        [[sc::thread_safe]] void set_force_dark_mode(bool enabled);
        [[sc::thread_safe]] void set_background(const color &color);

      public:
        [[sc::thread_safe]] void set_file(const fs::path &file);
        [[sc::thread_safe]] void set_url(const std::string &url);

      public:
        [[sc::thread_safe]] void embed(embedded_files files);
        [[sc::thread_safe]] void serve(const std::string &file);

      public:
        [[sc::thread_safe]] void clear_scripts();

      public:
        [[sc::thread_safe]] void clear_embedded();
        [[sc::thread_safe]] void clear_embedded(const std::string &file);

      public:
        [[sc::thread_safe]] void execute(const std::string &code);
        [[sc::thread_safe]] void inject(const std::string &code, load_time time, web_frame frame = web_frame::top);

      public:
        [[sc::thread_safe]] void handle_scheme(const std::string &name, scheme_handler handler);
        [[sc::thread_safe]] void remove_scheme(const std::string &name);

      public:
        using window::clear;
        [[sc::thread_safe]] void clear(web_event event);

        using window::remove;
        [[sc::thread_safe]] void remove(web_event event, std::uint64_t id);

        using window::once;
        template <web_event Event>
        [[sc::thread_safe]] void once(events::type<Event> callback);

        using window::on;
        template <web_event Event>
        [[sc::thread_safe]] std::uint64_t on(events::type<Event> callback);

      public:
        [[sc::before_init]] static void register_scheme(const std::string &name);
    };
} // namespace saucer
