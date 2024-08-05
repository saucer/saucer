#include "window.qt.impl.hpp"

#include <QThread>
#include <QCloseEvent>

namespace saucer
{
    window::impl::main_window::main_window(class window *parent) : m_parent(parent) {}

    void window::impl::main_window::changeEvent(QEvent *event)
    {
        QMainWindow::changeEvent(event);

        if (event->type() == QEvent::ActivationChange)
        {
            m_parent->m_events.at<window_event::focus>().fire(isActiveWindow());
            return;
        }

        if (event->type() != QEvent::WindowStateChange)
        {
            return;
        }

        auto *old = static_cast<QWindowStateChangeEvent *>(event);

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
        if (m_parent->m_events.at<window_event::close>().until(true))
        {
            event->ignore();
            return;
        }

        m_parent->m_events.at<window_event::closed>().fire();

        if (m_parent->m_impl->on_closed)
        {
            std::invoke(m_parent->m_impl->on_closed);
        }

        QMainWindow::closeEvent(event);
    }

    void window::impl::main_window::resizeEvent(QResizeEvent *event)
    {
        QMainWindow::resizeEvent(event);
        m_parent->m_events.at<window_event::resize>().fire(width(), height());
    }

    bool window::impl::event_receiver::event(QEvent *event)
    {
        if (event->type() != QEvent::User)
        {
            return QObject::event(event);
        }

        if (auto *cast = dynamic_cast<safe_event *>(event); cast)
        {
            std::invoke(cast->callback);
        }

        return QObject::event(event);
    }

    safe_event::safe_event(callback_t callback) : QEvent(QEvent::User), callback(std::move(callback)) {}

    bool window::impl::is_thread_safe() const
    {
        return QThread::currentThread() == window->thread();
    }
} // namespace saucer
