#pragma once
#include "window.hpp"

#include <QThread>
#include <optional>
#include <QCloseEvent>
#include <QMainWindow>
#include <QApplication>

namespace saucer
{
    struct window::impl
    {
        QMainWindow *window;

        bool is_thread_safe() const;
        template <typename Func> auto post_safe(Func &&);

        std::optional<QSize> max_size, min_size;

        class saucer_main_window;
        static inline QApplication *application;
    };

    class window::impl::saucer_main_window : public QMainWindow
    {
      private:
        class window *m_parent;

      public:
        saucer_main_window(class window *parent) : m_parent(parent) {}

      public:
        void resizeEvent(QResizeEvent *event) override
        {
            m_parent->m_events.at<window_event::resize>().fire(width(), height());
            QMainWindow::resizeEvent(event);
        }

        void closeEvent(QCloseEvent *event) override
        {
            for (const auto &result : m_parent->m_events.at<window_event::close>().fire())
            {
                if (result)
                {
                    event->ignore();
                    return;
                }
            }

            QMainWindow::closeEvent(event);
        }
    };

    inline bool window::impl::is_thread_safe() const
    {
        return QThread::currentThread() == window->thread();
    }

    template <typename Return> class event_callback : public QEvent
    {
        using callback_t = std::function<Return()>;

      private:
        callback_t m_func;
        std::promise<Return> &m_result;

      public:
        event_callback(callback_t &&func, std::promise<Return> &result) : QEvent(QEvent::None), m_func(func), m_result(result) {}
        ~event_callback() override
        {
            m_result.set_value(m_func());
        }
    };

    template <> class event_callback<void> : public QEvent
    {
        using callback_t = std::function<void()>;

      private:
        callback_t m_func;

      public:
        event_callback(callback_t &&func) : QEvent(QEvent::None), m_func(std::move(func)) {}
        ~event_callback() override
        {
            m_func();
        }
    };

    template <typename Func> auto window::impl::post_safe(Func &&func)
    {
        using return_t = typename decltype(std::function(func))::result_type;

        if constexpr (std::is_same_v<return_t, void>)
        {
            QApplication::postEvent(window, new event_callback<return_t>(std::function(func)));
        }
        else
        {
            std::promise<return_t> result;
            QApplication::postEvent(window, new event_callback<return_t>(std::function(func), result));

            return result.get_future().get();
        }
    }
} // namespace saucer