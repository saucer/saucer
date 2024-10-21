#include "qt.window.impl.hpp"

#include "instantiate.hpp"
#include "qt.icon.impl.hpp"

#include <cassert>

#include <fmt/core.h>
#include <fmt/xchar.h>
#include <flagpp/flags.hpp>

#include <QWindow>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    window::window(const preferences &prefs) : m_impl(std::make_unique<impl>()), m_parent(prefs.application.value())
    {
        assert(m_parent->thread_safe() && "Construction outside of the main-thread is not permitted");

        m_impl->window = std::make_unique<impl::main_window>(this);

        m_impl->max_size = m_impl->window->maximumSize();
        m_impl->min_size = m_impl->window->minimumSize();

        //? Fixes QT-Bug where Web-View will not render when background color is transparent.

        m_impl->set_alpha(255);
    }

    window::~window()
    {
        m_impl->window->disconnect();
        m_impl->window->close();
    }

    bool window::visible() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return visible(); });
        }

        return m_impl->window->isVisible();
    }

    bool window::focused() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return focused(); });
        }

        return m_impl->window->isActiveWindow();
    }

    bool window::minimized() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return minimized(); });
        }

        return m_impl->window->isMinimized();
    }

    bool window::maximized() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return maximized(); });
        }

        return m_impl->window->isMaximized();
    }

    bool window::resizable() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return resizable(); });
        }

        return m_impl->window->maximumSize() != m_impl->window->minimumSize();
    }

    bool window::decorations() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return decorations(); });
        }

        return !m_impl->window->windowFlags().testFlag(Qt::FramelessWindowHint);
    }

    std::string window::title() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return title(); });
        }

        return m_impl->window->windowTitle().toStdString();
    }

    bool window::always_on_top() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return always_on_top(); });
        }

        return m_impl->window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
    }

    std::pair<int, int> window::size() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return size(); });
        }

        return {m_impl->window->width(), m_impl->window->height()};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return max_size(); });
        }

        return {m_impl->window->maximumWidth(), m_impl->window->maximumHeight()};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return min_size(); });
        }

        return {m_impl->window->minimumWidth(), m_impl->window->minimumHeight()};
    }

    void window::hide()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return hide(); });
        }

        m_impl->window->hide();
    }

    void window::show()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return show(); });
        }

        m_impl->window->show();
    }

    void window::close()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return close(); });
        }

        m_impl->window->close();
    }

    void window::focus()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return focus(); });
        }

        m_impl->window->activateWindow();
    }

    void window::start_drag()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return start_drag(); });
        }

        m_impl->window->windowHandle()->startSystemMove();
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, edge] { return start_resize(edge); });
        }

        Qt::Edges translated;

        if (edge & window_edge::top)
        {
            translated |= Qt::Edge::TopEdge;
        }
        if (edge & window_edge::bottom)
        {
            translated |= Qt::Edge::BottomEdge;
        }
        if (edge & window_edge::left)
        {
            translated |= Qt::Edge::LeftEdge;
        }
        if (edge & window_edge::right)
        {
            translated |= Qt::Edge::RightEdge;
        }

        m_impl->window->windowHandle()->startSystemResize(translated);
    }

    void window::set_minimized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_minimized(enabled); });
        }

        auto state = m_impl->window->windowState();

        if (enabled)
        {
            state |= Qt::WindowState::WindowMinimized;
        }
        else
        {
            state &= ~Qt::WindowState::WindowMinimized;
            state |= Qt::WindowState::WindowActive;
        }

        m_impl->window->setWindowState(state);
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_maximized(enabled); });
        }

        auto state = m_impl->window->windowState();

        if (enabled)
        {
            state |= Qt::WindowState::WindowMaximized;
        }
        else
        {
            state &= ~Qt::WindowState::WindowMaximized;
        }

        m_impl->window->setWindowState(state);
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(enabled); });
        }

        if (!enabled)
        {
            m_impl->window->setFixedSize(m_impl->window->size());
            return;
        }

        m_impl->window->setMaximumSize(m_impl->max_size);
        m_impl->window->setMinimumSize(m_impl->min_size);
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(enabled); });
        }

        m_impl->window->setWindowFlag(Qt::FramelessWindowHint, !enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_always_on_top(enabled); });
        }

        m_impl->window->setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!m_parent->thread_safe())
        {
            return dispatch([this, icon] { return set_icon(icon); });
        }

        m_impl->window->setWindowIcon(icon.m_impl->icon);
    }

    void window::set_title(const std::string &title)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, title] { return set_title(title); });
        }

        m_impl->window->setWindowTitle(QString::fromStdString(title));
    }

    void window::set_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_size(width, height); });
        }

        m_impl->window->resize(width, height);
    }

    void window::set_max_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_max_size(width, height); });
        }

        m_impl->window->setMaximumSize(width, height);
        m_impl->max_size = m_impl->window->maximumSize();
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_min_size(width, height); });
        }

        m_impl->window->setMinimumSize(width, height);
        m_impl->min_size = m_impl->window->minimumSize();
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::type<Event> callback)
    {
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type<Event> callback)
    {
        return m_events.at<Event>().add(std::move(callback));
    }

    INSTANTIATE_EVENTS(window, 7, window_event)
} // namespace saucer
