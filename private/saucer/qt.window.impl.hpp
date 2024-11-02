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
        void set_alpha(std::uint8_t alpha) const;
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
} // namespace saucer
