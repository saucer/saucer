#pragma once
#include <map>
#include "window.hpp"
#include "events/events.hpp"

namespace saucer
{
    enum class load_time
    {
        ready,
        creation,
    };

    enum class web_event
    {
        url_changed
    };

#include "annotations.hpp"
    class webview : public window
    {
        struct impl;
        using embedded_files = std::map<const std::string, std::tuple<std::string, std::size_t, const std::uint8_t *>>;

        using events = event_handler<                                //
            event<web_event::url_changed, void(const std::string &)> //
            >;

      private:
        events m_events;
        embedded_files m_embedded_files;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        virtual void on_message(const std::string &);
        virtual void on_url_changed(const std::string &);

      public:
        webview();
        ~webview() override;

      public:
        [[thread_safe]] bool get_dev_tools() const;
        [[thread_safe]] std::string get_url() const;
        [[thread_safe]] bool get_transparent() const;
        [[thread_safe]] bool get_context_menu() const;

      public:
        void serve_embedded(const std::string &file);
        [[thread_safe]] void set_dev_tools(bool enabled);
        [[thread_safe]] void set_transparent(bool enabled);
        [[thread_safe]] void set_context_menu(bool enabled);
        [[thread_safe]] void set_url(const std::string &url);
        [[thread_safe]] void embed_files(embedded_files &&files);

      public:
        [[thread_safe]] void clear_scripts();
        [[thread_safe]] void clear_embedded();
        [[thread_safe]] void run_java_script(const std::string &java_script);
        [[thread_safe]] void inject(const std::string &java_script, const load_time &load_time);

      public:
        using window::clear;
        [[thread_safe]] void clear(web_event event);

        using window::unregister;
        [[thread_safe]] void unregister(web_event event, std::size_t id);

        using window::on;
        template <web_event Event> //
        [[thread_safe]] std::size_t on(events::get_t<Event> &&callback) = delete;
    };
#include "annotations.hpp" //NOLINT

    template <> std::size_t webview::on<web_event::url_changed>(events::get_t<web_event::url_changed> &&callback);
} // namespace saucer