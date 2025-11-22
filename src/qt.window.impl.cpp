#include "qt.window.impl.hpp"

#include <QCloseEvent>

namespace saucer
{
    using native = window::impl::native;
    using event  = window::event;

    void native::set_flags(std::initializer_list<std::pair<Qt::WindowType, bool>> flags) const
    {
        const auto shown = window->isVisible();

        for (const auto &[flag, enabled] : flags)
        {
            window->setWindowFlag(flag, enabled);
        }

        if (!shown || shown == window->isVisible())
        {
            return;
        }

        window->show();
    }

    void native::add_widget(QWidget *widget) const
    {
        window->centralWidget()->layout()->addWidget(widget);
    }

    void native::remove_widget(QWidget *widget) const
    {
        window->centralWidget()->layout()->removeWidget(widget);
    }

    main_window::main_window(window::impl *impl) : impl(impl) {}

    void main_window::changeEvent(QEvent *event)
    {
        QMainWindow::changeEvent(event);

        if (event->type() == QEvent::ActivationChange)
        {
            impl->events.get<event::focus>().fire(isActiveWindow());
            return;
        }

        if (event->type() != QEvent::WindowStateChange)
        {
            return;
        }

        auto *const old = static_cast<QWindowStateChangeEvent *>(event);

        if (old->oldState().testFlag(Qt::WindowState::WindowMaximized) != isMaximized())
        {
            impl->events.get<event::maximize>().fire(isMaximized());
        }

        if (old->oldState().testFlag(Qt::WindowState::WindowMinimized) != isMinimized())
        {
            impl->events.get<event::minimize>().fire(isMinimized());
        }
    }

    void main_window::closeEvent(QCloseEvent *event)
    {
        if (impl->events.get<event::close>().fire().find(policy::block))
        {
            event->ignore();
            return;
        }

        QMainWindow::closeEvent(event);
        impl->events.get<event::closed>().fire();
    }

    void main_window::resizeEvent(QResizeEvent *event)
    {
        QMainWindow::resizeEvent(event);
        impl->events.get<event::resize>().fire(width(), height());
    }

    overlay_layout::overlay_layout(window::impl *impl) : QLayout(), impl(impl) {}

    overlay_layout::~overlay_layout()
    {
        for (auto *const item : m_items)
        {
            delete item;
        }

        m_items.clear();
    }

    int overlay_layout::count() const
    {
        return static_cast<int>(m_items.size());
    }

    QLayoutItem *overlay_layout::takeAt(int index)
    {
        if (static_cast<std::size_t>(index) >= m_items.size())
        {
            return nullptr;
        }

        auto *const item = m_items[index];
        m_items.erase(m_items.begin() + index);

        return item;
    }

    QLayoutItem *overlay_layout::itemAt(int index) const
    {
        if (static_cast<std::size_t>(index) >= m_items.size())
        {
            return nullptr;
        }

        return m_items[index];
    }

    QSize overlay_layout::sizeHint() const
    {
        return geometry().size();
    }

    QSize overlay_layout::minimumSize() const
    {
        return {};
    }

    void overlay_layout::addItem(QLayoutItem *item)
    {
        m_items.emplace_back(item);
    }

    void overlay_layout::setGeometry(const QRect &rect)
    {
        for (auto *const item : m_items)
        {
            const auto resize = m_initialized && !impl->platform->unmanaged.contains(item->widget());

            if (!resize)
            {
                continue;
            }

            item->setGeometry(rect);
        }

        m_initialized = true;
        QLayout::setGeometry(rect);
    }
} // namespace saucer
