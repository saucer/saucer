#pragma once
#include <functional>
#include <memory>
#include <string>

namespace saucer
{
    class window
    {
        struct impl;
        using close_callback_t = std::function<bool()>;
        using resize_callback_t = std::function<void(std::size_t, std::size_t)>;

      protected:
        std::unique_ptr<impl> m_impl;
        close_callback_t m_close_callback;
        resize_callback_t m_resize_callback;

      protected:
        window();

      public:
        virtual ~window();

      public:
        bool get_resizeable() const;
        bool get_decorations() const;
        std::string get_title() const;
        bool get_always_on_top() const;
        std::pair<std::size_t, std::size_t> get_size() const;
        std::pair<std::size_t, std::size_t> get_max_size() const;
        std::pair<std::size_t, std::size_t> get_min_size() const;

      public:
        virtual void run();
        virtual void hide();
        virtual void show();

      public:
        void on_close(const close_callback_t &callback);
        void on_resize(const resize_callback_t &callback);

      public:
        virtual void set_resizeable(bool enabled);
        virtual void set_decorations(bool enabled);
        virtual void set_title(const std::string &);
        virtual void set_always_on_top(bool enabled);
        virtual void set_size(std::size_t width, std::size_t height);
        virtual void set_max_size(std::size_t width, std::size_t height);
        virtual void set_min_size(std::size_t width, std::size_t height);
    };
} // namespace saucer