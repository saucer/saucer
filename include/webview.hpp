#pragma once
#include <map>
#include <window.hpp>

namespace saucer
{
    enum class load_time_t
    {
        ready,
        creation,
    };

    class webview : public window
    {
        struct impl;
        using url_changed_callback_t = std::function<void(const std::string &)>;
        using embedded_files = std::map<const std::string, std::tuple<std::string, std::size_t, const std::uint8_t *>>;

      private:
        embedded_files m_embedded_files;
        url_changed_callback_t m_url_changed_callback;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        virtual void on_message(const std::string &);

      public:
        webview();
        ~webview() override;

      public:
        bool get_dev_tools() const SAUCER_THREAD_SAFE;
        std::string get_url() const;
        bool get_context_menu() const;

      public:
        void set_dev_tools(bool enabled) SAUCER_THREAD_SAFE;
        void set_context_menu(bool enabled);
        void set_url(const std::string &url) SAUCER_THREAD_SAFE;
        void serve_embedded(const std::string &file);
        void embed_files(const embedded_files &files) SAUCER_THREAD_SAFE;

      public:
        void run_java_script(const std::string &java_script) SAUCER_THREAD_SAFE;
        void inject(const std::string &java_script, const load_time_t &load_time) SAUCER_THREAD_SAFE;

      public:
        void clear_scripts() SAUCER_THREAD_SAFE;
        void clear_embedded() SAUCER_THREAD_SAFE;

      public:
        void on_url_changed(const url_changed_callback_t &callback);
    };
} // namespace saucer