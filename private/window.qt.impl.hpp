#pragma once

#include "window.hpp"

#include <QMainWindow>
#include <QApplication>

namespace saucer
{
    struct window::impl
    {
        class main_window;
        struct event_receiver;

      public:
        std::unique_ptr<QMainWindow> window;

      public:
        QSize max_size, min_size;
        std::function<void()> on_closed;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        static thread_local inline std::unique_ptr<QApplication> application;
        static inline std::unique_ptr<QObject> receiver;
    };

    class window::impl::main_window : public QMainWindow
    {
        saucer::window *m_parent;

      public:
        main_window(saucer::window *parent);

      public:
        void changeEvent(QEvent *event) override;

      public:
        void closeEvent(QCloseEvent *event) override;

      public:
        void resizeEvent(QResizeEvent *event) override;
    };

    struct safe_event : QEvent
    {
        using callback_t = std::move_only_function<void()>;

      public:
        callback_t callback;

      public:
        safe_event(callback_t callback);
    };

    struct window::impl::event_receiver : public QObject
    {
        bool event(QEvent *) override;
    };
} // namespace saucer
