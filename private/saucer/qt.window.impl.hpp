#pragma once

#include "window.impl.hpp"

#include <QLayout>
#include <QMainWindow>

namespace saucer
{
    struct window::impl::native
    {
        QMainWindow *window;

      public:
        QSize max_size, min_size;

      public:
        void set_flags(std::initializer_list<std::pair<Qt::WindowType, bool>> flags) const;

      public:
        void add_widget(QWidget *) const;
        void remove_widget(QWidget *) const;
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

    class overlay_layout : public QLayout
    {
        bool m_initialized{false};
        std::vector<QLayoutItem *> m_items;

      public:
        using QLayout::QLayout;

      public:
        ~overlay_layout() override;

      public:
        [[nodiscard]] int count() const override;
        [[nodiscard]] QLayoutItem *takeAt(int) override;
        [[nodiscard]] QLayoutItem *itemAt(int) const override;

      public:
        [[nodiscard]] QSize sizeHint() const override;
        [[nodiscard]] QSize minimumSize() const override;

      public:
        void addItem(QLayoutItem *) override;
        void setGeometry(const QRect &) override;
    };
} // namespace saucer
