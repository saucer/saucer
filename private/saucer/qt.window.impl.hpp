#pragma once

#include "window.impl.hpp"

#include "qt.utils.hpp"

#include <unordered_set>

#include <QLayout>
#include <QMainWindow>

namespace saucer
{
    struct window::impl::native
    {
        utils::deferred_ptr<QMainWindow> window;

      public:
        QSize max_size, min_size;
        std::unordered_set<QWidget *> unmanaged;

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
        window::impl *impl;

      private:
        bool m_initialized{false};
        std::vector<QLayoutItem *> m_items;

      public:
        overlay_layout(window::impl *);

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
