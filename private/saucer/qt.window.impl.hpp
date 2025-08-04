#pragma once

#include "window.impl.hpp"

#include <QMainWindow>

namespace saucer
{
    struct window::impl::native
    {
        std::unique_ptr<QMainWindow> window;

      public:
        QSize max_size, min_size;
        std::move_only_function<void()> on_closed;

      public:
        void set_flags(std::initializer_list<std::pair<Qt::WindowType, bool>> flags) const;
    };

    struct main_window : QMainWindow
    {
        window::impl *impl;

      public:
        main_window(window::impl *);

      public:
        void changeEvent(QEvent *) override;
        void closeEvent(QCloseEvent *) override;
        void resizeEvent(QResizeEvent *) override;
    };
} // namespace saucer
