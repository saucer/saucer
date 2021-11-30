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

    struct embedded_file
    {
        const std::string mime;
        const std::size_t size;
        const std::uint8_t *data;
    };

    class webview : public window
    {
        struct impl;
        using embedded_files = std::map<const std::string, embedded_file>;
        using url_changed_callback_t = std::function<void(const std::string &)>;

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
        bool get_dev_tools() const;
        std::string get_url() const;
        bool get_context_menu() const;

      public:
        void set_dev_tools(bool enabled);
        void set_context_menu(bool enabled);
        void set_url(const std::string &url);
        void serve_embedded(const std::string &file);
        void embed_files(const embedded_files &files);

      public:
        void run_java_script(const std::string &java_script);
        void inject(const std::string &java_script, const load_time_t &load_time);

      public:
        void clear_scripts();
        void clear_embedded();

      public:
        void on_url_changed(const url_changed_callback_t &callback);
    };
} // namespace saucer