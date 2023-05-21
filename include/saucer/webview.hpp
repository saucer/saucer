#pragma once
#include "window.hpp"

#include <map>
#include <filesystem>
#include <ereignis/manager.hpp>

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

    struct embedded_file
    {
        const std::string mime;
        const std::size_t size;
        const std::uint8_t *data;
    };

#include "annotations.hpp"
    class webview : public window
    {
        struct impl;

      private:
        using events = ereignis::event_manager<                                //
            ereignis::event<web_event::url_changed, void(const std::string &)> //
            >;

      private:
        events m_events;
        std::map<const std::string, const embedded_file> m_embedded_files;

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
        [[thread_safe]] [[nodiscard]] bool get_dev_tools() const;
        [[thread_safe]] [[nodiscard]] std::string get_url() const;
        [[thread_safe]] [[nodiscard]] bool get_context_menu() const;

      public:
        [[thread_safe]] void set_dev_tools(bool enabled);
        [[thread_safe]] void set_context_menu(bool enabled);
        [[thread_safe]] void set_url(const std::string &url);

      public:
        [[thread_safe]] void serve(const std::string &file);
        [[thread_safe]] void embed(std::map<const std::string, const embedded_file> &&files);

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
        template <web_event Event> //
        [[thread_safe]] std::uint64_t on(events::callback_t<Event> &&callback) = delete;
    };
#include "annotations.hpp" //NOLINT

    template <>
    std::uint64_t webview::on<web_event::url_changed>(events::callback_t<web_event::url_changed> &&callback);
} // namespace saucer