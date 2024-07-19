#pragma once

#include "window.hpp"

#include <map>
#include <span>
#include <string>
#include <memory>

#include <ereignis/manager.hpp>

namespace saucer
{
    enum class load_time : std::uint8_t
    {
        creation,
        ready,
    };

    enum class web_event : std::uint8_t
    {
        load_finished,
        load_started,
        url_changed,
        dom_ready,
    };

    struct embedded_file
    {
        std::string mime;
        std::span<const std::uint8_t> content;
    };

    class webview : public window
    {
        struct impl;

      private:
        using embedded_files = std::map<std::string, embedded_file>;

      private:
        using events = ereignis::manager<                                       //
            ereignis::event<web_event::load_finished, void()>,                  //
            ereignis::event<web_event::load_started, void()>,                   //
            ereignis::event<web_event::url_changed, void(const std::string &)>, //
            ereignis::event<web_event::dom_ready, void()>                       //
            >;

      private:
        events m_events;
        embedded_files m_embedded_files;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        virtual bool on_message(const std::string &);

      public:
        webview(const options & = {});

      public:
        ~webview() override;

      public:
        [[sc::thread_safe]] [[nodiscard]] bool dev_tools() const;
        [[sc::thread_safe]] [[nodiscard]] std::string url() const;
        [[sc::thread_safe]] [[nodiscard]] bool context_menu() const;

      public:
        [[sc::thread_safe]] void set_dev_tools(bool enabled);
        [[sc::thread_safe]] void set_context_menu(bool enabled);
        [[sc::thread_safe]] void set_url(const std::string &url);
        [[sc::thread_safe]] void set_file(const std::string &file);

      public:
        [[sc::thread_safe]] void embed(embedded_files &&files);
        [[sc::thread_safe]] void serve(const std::string &file);

      public:
        [[sc::thread_safe]] void clear_scripts();
        [[sc::thread_safe]] void clear_embedded();

      public:
        [[sc::thread_safe]] void execute(const std::string &java_script);
        [[sc::thread_safe]] void inject(const std::string &java_script, const load_time &load_time);

      public:
        using window::clear;
        [[sc::thread_safe]] void clear(web_event event);

        using window::remove;
        [[sc::thread_safe]] void remove(web_event event, std::uint64_t id);

        using window::once;
        template <web_event Event>
        [[sc::thread_safe]] void once(events::type_t<Event> &&callback);

        using window::on;
        template <web_event Event>
        [[sc::thread_safe]] std::uint64_t on(events::type_t<Event> &&callback);
    };
} // namespace saucer
