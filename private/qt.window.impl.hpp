#pragma once

#include "window.hpp"

#include <QMainWindow>
#include <QApplication>

namespace saucer
{
    struct window::impl
    {
        class main_window;

      public:
        std::unique_ptr<QMainWindow> window;

      public:
        QSize max_size, min_size;
        std::function<void()> on_closed;

      public:
        [[nodiscard]] static bool is_thread_safe();

      public:
        static thread_local inline std::unique_ptr<QApplication> application;
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

    class safe_event : public QEvent
    {
        using callback_t = std::move_only_function<void()>;

      private:
        callback_t m_callback;

      public:
        safe_event(callback_t callback);

      public:
        ~safe_event() override;
    };
} // namespace saucer
