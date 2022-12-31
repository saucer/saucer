#include "window.hpp"
#include "window.qt.impl.hpp"

namespace saucer
{
    window::window() : m_impl(std::make_unique<impl>())
    {
        static int argc{1};
        static char *argv{strdup("")};

        if (!m_impl->application)
        {
            qputenv("QT_LOGGING_RULES", "*=false");
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

            m_impl->application = new QApplication(argc, &argv);
        }

        m_impl->window = new impl::saucer_main_window(this);

        //? Fixes QT-Bug where Web-View will not render when background color is transparent.
        auto palette = m_impl->window->palette();
        palette.setColor(QPalette::ColorRole::Window, QColor(255, 255, 255));

        m_impl->window->setPalette(palette);
    }

    window::~window()
    {
        m_impl->window->deleteLater();
    }

    bool window::get_resizable() const
    {
        return m_impl->window->maximumSize() != m_impl->window->minimumSize();
    }

    std::string window::get_title() const
    {
        return m_impl->window->windowTitle().toStdString();
    }

    bool window::get_decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_decorations(); });
        }
        return !m_impl->window->windowFlags().testFlag(Qt::FramelessWindowHint);
    }

    bool window::get_always_on_top() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_always_on_top(); });
        }
        return m_impl->window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
    }

    std::pair<std::size_t, std::size_t> window::get_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_size(); });
        }
        return {m_impl->window->width(), m_impl->window->height()};
    }

    std::pair<std::size_t, std::size_t> window::get_max_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_max_size(); });
        }
        return {m_impl->window->maximumWidth(), m_impl->window->maximumHeight()};
    }

    std::pair<std::size_t, std::size_t> window::get_min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_min_size(); });
        }
        return {m_impl->window->minimumWidth(), m_impl->window->minimumHeight()};
    }

    std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> window::get_background_color() const
    {
        const auto color = m_impl->window->palette().color(QPalette::ColorRole::Window);
        return {color.red(), color.green(), color.blue(), color.alpha()};
    }

    void window::hide()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return hide(); });
        }
        m_impl->window->hide();
    }

    void window::show()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return show(); });
        }
        m_impl->window->show();
    }

    void window::close()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return close(); });
        }
        m_impl->window->close();
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_resizable(enabled); });
        }

        if (!enabled)
        {
            m_impl->window->setFixedSize(m_impl->window->size());
        }
        else
        {
            m_impl->window->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

            if (m_impl->max_size)
            {
                m_impl->window->setMaximumSize(*m_impl->max_size);
            }
            if (m_impl->min_size)
            {
                m_impl->window->setMinimumSize(*m_impl->min_size);
            }
        }
    }

    void window::set_title(const std::string &title)
    {
        m_impl->window->setWindowTitle(QString::fromStdString(title));
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_decorations(enabled); });
        }
        m_impl->window->setWindowFlag(Qt::FramelessWindowHint, !enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_always_on_top(enabled); });
        }
        m_impl->window->setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    }

    void window::set_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_size(width, height); });
        }
        m_impl->window->resize(static_cast<int>(width), static_cast<int>(height));
    }

    void window::set_max_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_max_size(width, height); });
        }
        m_impl->window->setMaximumSize(static_cast<int>(width), static_cast<int>(height));
        m_impl->max_size = m_impl->window->maximumSize();
    }

    void window::set_min_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { return set_min_size(width, height); });
        }
        m_impl->window->setMinimumSize(static_cast<int>(width), static_cast<int>(height));
        m_impl->min_size = m_impl->window->minimumSize();
    }

    void window::set_background_color(std::size_t r, std::size_t g, std::size_t b, std::size_t a)
    {
        auto palette = m_impl->window->palette();
        palette.setColor(QPalette::ColorRole::Window, QColor(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a)));

        m_impl->window->setPalette(palette);
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <> std::uint64_t window::on<window_event::close>(events::callback_t<window_event::close> &&callback)
    {
        return m_events.at<window_event::close>().add(std::move(callback));
    }

    template <> std::uint64_t window::on<window_event::resize>(events::callback_t<window_event::resize> &&callback)
    {
        return m_events.at<window_event::resize>().add(std::move(callback));
    }

    void window::run()
    {
        QApplication::exec();
    }
} // namespace saucer