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

    class webview : public window
    {
        struct impl;
        using url_changed_callback_t = std::function<void(const std::string &)>;

      private:
        url_changed_callback_t m_url_changed_callback;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        virtual void on_message(const std::string &);

      public:
        webview();
        ~webview() override;

      public:
        std::string get_url() const;
        bool get_context_menu() const;

      public:
        void set_context_menu(bool enabled);
        void set_url(const std::string &url);

      public:
        void run_java_script(const std::string &java_script);
        void inject(const std::string &java_script, const load_time_t &load_time);

      public:
        void clear_scripts();

      public:
        void on_url_changed(const url_changed_callback_t &callback);
    };

    template <typename bridge_t, std::enable_if_t<std::is_base_of_v<window, bridge_t>> * = nullptr> using bridged_webview = bridge_t;
} // namespace saucer