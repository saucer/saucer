#include "qt.window.impl.hpp"

#include <QThread>
#include <QCloseEvent>

namespace saucer
{
    void window::impl::set_alpha(std::uint8_t alpha) const
    {
        auto palette = window->palette();

        auto color = palette.color(QPalette::ColorRole::Window);
        color.setAlpha(alpha);

        palette.setColor(QPalette::ColorRole::Window, color);

        window->setAttribute(Qt::WA_TranslucentBackground, alpha < 255);
        window->setPalette(palette);
    }

    void window::impl::set_flag(Qt::WindowType flag, bool enabled) const
    {
        const auto shown = window->isVisible();
        window->setWindowFlag(flag, enabled);

        if (!shown || shown == window->isVisible())
        {
            return;
        }

        window->show();
    }

    window::impl::main_window::main_window(saucer::window *parent) : m_parent(parent) {}

    void window::impl::main_window::changeEvent(QEvent *event)
    {
        QMainWindow::changeEvent(event);

        if (event->type() == QEvent::ParentChange)
        {
            m_parent->m_events.at<window_event::decorated>().fire(m_parent->decorations());
            return;
        }

        if (event->type() == QEvent::ActivationChange)
        {
            m_parent->m_events.at<window_event::focus>().fire(isActiveWindow());
            return;
        }

        if (event->type() != QEvent::WindowStateChange)
        {
            return;
        }

        auto *const old = static_cast<QWindowStateChangeEvent *>(event);

        if (old->oldState().testFlag(Qt::WindowState::WindowMaximized) != isMaximized())
        {
            m_parent->m_events.at<window_event::maximize>().fire(isMaximized());
        }

        if (old->oldState().testFlag(Qt::WindowState::WindowMinimized) != isMinimized())
        {
            m_parent->m_events.at<window_event::minimize>().fire(isMinimized());
        }
    }

    void window::impl::main_window::closeEvent(QCloseEvent *event)
    {
        if (m_parent->m_events.at<window_event::close>().until(policy::block))
        {
            event->ignore();
            return;
        }

        if (m_parent->m_impl->on_closed)
        {
            std::invoke(m_parent->m_impl->on_closed);
        }

        m_parent->m_events.at<window_event::closed>().fire();

        QMainWindow::closeEvent(event);
    }

    void window::impl::main_window::resizeEvent(QResizeEvent *event)
    {
        QMainWindow::resizeEvent(event);
        m_parent->m_events.at<window_event::resize>().fire(width(), height());
    }
} // namespace saucer
