#pragma once
#include <window.hpp>

namespace saucer
{
    enum class load_time_t
    {
        done,
        ready,
        creation,
    };

    class webview final : public window
    {
        struct impl;
        using message_callback_t = std::function<void(const std::string &)>;
        using url_changed_callback_t = std::function<void(const std::string &)>;

      protected:
        std::unique_ptr<impl> m_impl;
        message_callback_t m_message_callback;
        url_changed_callback_t m_url_changed_callback;

      public:
        webview();
        ~webview() final;

      public:
        webview(webview &&) = delete;
        webview(const webview &) = delete;

      public:
        std::string get_url() const;
        bool get_context_menu() const;

      public:
        void set_context_menu(bool enabled);
        void set_url(const std::string &url);

      public:
        void call(const std::string &java_script);
        void inject(const std::string &java_script, const load_time_t &load_time);

      public:
        void clear_scripts();

      public:
        void set_message_callback(const message_callback_t &callback);
        void set_url_changed_callback(const url_changed_callback_t &callback);
    };
} // namespace saucer