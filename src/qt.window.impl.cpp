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

    main_window::main_window(window::impl *impl) : impl(impl) {}

    void main_window::changeEvent(QEvent *event)
    {
        QMainWindow::changeEvent(event);

        if (event->type() == QEvent::ParentChange)
        {
            impl->events->get<event::decorated>().fire(impl->decorations());
            return;
        }

        if (event->type() == QEvent::ActivationChange)
        {
            impl->events->get<event::focus>().fire(isActiveWindow());
            return;
        }

        if (event->type() != QEvent::WindowStateChange)
        {
            return;
        }

        auto *const old = static_cast<QWindowStateChangeEvent *>(event);

        if (old->oldState().testFlag(Qt::WindowState::WindowMaximized) != isMaximized())
        {
            impl->events->get<event::maximize>().fire(isMaximized());
        }

        if (old->oldState().testFlag(Qt::WindowState::WindowMinimized) != isMinimized())
        {
            impl->events->get<event::minimize>().fire(isMinimized());
        }
    }

    void main_window::closeEvent(QCloseEvent *event)
    {
        if (impl->events->get<event::close>().fire().find(policy::block))
        {
            event->ignore();
            return;
        }

        if (impl->platform->on_closed)
        {
            std::invoke(impl->platform->on_closed);
        }

        QMainWindow::closeEvent(event);
        impl->events->get<event::closed>().fire();
    }

    void main_window::resizeEvent(QResizeEvent *event)
    {
        QMainWindow::resizeEvent(event);
        impl->events->get<event::resize>().fire(width(), height());
    }
} // namespace saucer
