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
        ready,
        creation,
    };

    enum class web_event : std::uint8_t
    {
        url_changed
    };

    struct embedded_file
    {
        std::string mime;
        std::span<const std::uint8_t> content;
    };

#include "meta/annotations.hpp"
    class webview : public window
    {
        struct impl;

      private:
        using embedded_files = std::map<std::string, embedded_file>;

      private:
        using events = ereignis::manager<                                      //
            ereignis::event<web_event::url_changed, void(const std::string &)> //
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
        webview(const options & = {});

      public:
        ~webview() override;

      public:
        [[thread_safe]] [[nodiscard]] bool dev_tools() const;
        [[thread_safe]] [[nodiscard]] std::string url() const;
        [[thread_safe]] [[nodiscard]] bool context_menu() const;

      public:
        [[thread_safe]] void set_dev_tools(bool enabled);
        [[thread_safe]] void set_context_menu(bool enabled);
        [[thread_safe]] void set_url(const std::string &url);

      public:
        [[thread_safe]] void embed(embedded_files &&files);
        [[thread_safe]] void serve(const std::string &file);

      public:
        [[thread_safe]] void clear_scripts();
        [[thread_safe]] void clear_embedded();
        [[thread_safe]] void run_java_script(const std::string &java_script);
        [[thread_safe]] void inject(const std::string &java_script, const load_time &load_time);

      public:
        using window::clear;
        [[thread_safe]] void clear(web_event event);

        using window::remove;
        [[thread_safe]] void remove(web_event event, std::uint64_t id);

        using window::on;
        template <web_event Event>
        [[thread_safe]] std::uint64_t on(events::type_t<Event> &&callback) = delete;
    };
#include "meta/annotations.hpp" //NOLINT

    template <>
    std::uint64_t webview::on<web_event::url_changed>(events::type_t<web_event::url_changed> &&callback);
} // namespace saucer