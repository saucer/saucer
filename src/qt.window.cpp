#include "qt.window.impl.hpp"

#include "qt.app.impl.hpp"
#include "qt.icon.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <flagpp/flags.hpp>

#include <QWindow>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    window::window(application *parent) : m_parent(parent), m_impl(std::make_unique<impl>())
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

    template <window_event Event>
    void window::setup()
    {
    }

    bool window::visible() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return visible(); });
        }

        return m_impl->window->isVisible();
    }

    bool window::focused() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focused(); });
        }

        return m_impl->window->isActiveWindow();
    }

    bool window::minimized() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return minimized(); });
        }

        return m_impl->window->isMinimized();
    }

    bool window::maximized() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return maximized(); });
        }

        return m_impl->window->isMaximized();
    }

    bool window::resizable() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return resizable(); });
        }

        return m_impl->window->maximumSize() != m_impl->window->minimumSize();
    }

    window_decoration window::decoration() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return decoration(); });
        }

        if (m_impl->window->windowFlags().testFlag(Qt::FramelessWindowHint))
        {
            return window_decoration::none;
        }

        if (m_impl->window->windowFlags().testFlag(Qt::CustomizeWindowHint))
        {
            return window_decoration::partial;
        }

        return window_decoration::full;
    }

    std::string window::title() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return title(); });
        }

        return m_impl->window->windowTitle().toStdString();
    }

    bool window::always_on_top() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return always_on_top(); });
        }

        return m_impl->window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
    }

    bool window::click_through() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return click_through(); });
        }

        return m_impl->window->windowFlags().testFlag(Qt::WindowTransparentForInput);
    }

    std::pair<int, int> window::size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return size(); });
        }

        return {m_impl->window->width(), m_impl->window->height()};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return max_size(); });
        }

        return {m_impl->window->maximumWidth(), m_impl->window->maximumHeight()};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return min_size(); });
        }

        return {m_impl->window->minimumWidth(), m_impl->window->minimumHeight()};
    }

    std::pair<int, int> window::position() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return position(); });
        }

        return {m_impl->window->x(), m_impl->window->y()};
    }

    std::optional<saucer::screen> window::screen() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return screen(); });
        }

        auto *const screen = m_impl->window->screen();

        if (!screen)
        {
            return std::nullopt;
        }

        return application::impl::convert(screen);
    }

    void window::hide()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return hide(); });
        }

        m_impl->window->hide();
    }

    void window::show()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return show(); });
        }

        m_impl->window->show();
    }

    void window::close()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return close(); });
        }

        m_impl->window->close();
    }

    void window::focus()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focus(); });
        }

        m_impl->window->activateWindow();
    }

    void window::start_drag()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return start_drag(); });
        }

        m_impl->window->windowHandle()->startSystemMove();
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, edge] { return start_resize(edge); });
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
            return m_parent->dispatch([this, enabled] { return set_minimized(enabled); });
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
            return m_parent->dispatch([this, enabled] { return set_maximized(enabled); });
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
            return m_parent->dispatch([this, enabled] { return set_resizable(enabled); });
        }

        if (!enabled)
        {
            m_impl->window->setFixedSize(m_impl->window->size());
            return;
        }

        m_impl->window->setMaximumSize(m_impl->max_size);
        m_impl->window->setMinimumSize(m_impl->min_size);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_always_on_top(enabled); });
        }

        m_impl->set_flags({{Qt::WindowStaysOnTopHint, enabled}});
    }

    void window::set_click_through(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_click_through(enabled); });
        }

        m_impl->set_flags({{Qt::WindowTransparentForInput, enabled}});
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, icon] { return set_icon(icon); });
        }

        m_impl->window->setWindowIcon(icon.native<false>()->icon);
    }

    void window::set_title(const std::string &title)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, title] { return set_title(title); });
        }

        m_impl->window->setWindowTitle(QString::fromStdString(title));
    }

    void window::set_decoration(window_decoration decoration)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, decoration] { return set_decoration(decoration); });
        }

        m_impl->set_flags({
            {Qt::CustomizeWindowHint, decoration == window_decoration::partial},
            {Qt::FramelessWindowHint, decoration == window_decoration::none},
        });
    }

    void window::set_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_size(width, height); });
        }

        m_impl->window->resize(width, height);
    }

    void window::set_max_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_max_size(width, height); });
        }

        m_impl->window->setMaximumSize(width, height);
        m_impl->max_size = m_impl->window->maximumSize();
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_min_size(width, height); });
        }

        m_impl->window->setMinimumSize(width, height);
        m_impl->min_size = m_impl->window->minimumSize();
    }

    void window::set_position(int x, int y)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, x, y] { return set_position(x, y); });
        }

        m_impl->window->move(x, y);
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS;
} // namespace saucer
