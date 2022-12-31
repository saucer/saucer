#include "window.qt.impl.hpp"

#include <QThread>

namespace saucer
{
    QApplication *window::impl::application;

    window::impl::main_window::main_window(class window *parent) : m_parent(parent) {}

    void window::impl::main_window::closeEvent(QCloseEvent *event)
    {
        for (const auto &result : m_parent->m_events.at<window_event::close>().fire())
        {
            if (result)
            {
                event->ignore();
                return;
            }
        }

        QMainWindow::closeEvent(event);
    }

    void window::impl::main_window::resizeEvent(QResizeEvent *event)
    {
        m_parent->m_events.at<window_event::resize>().fire(width(), height());
        QMainWindow::resizeEvent(event);
    }

    bool window::impl::is_thread_safe() const
    {
        return QThread::currentThread() == window->thread();
    }
} // namespace saucer