#include "window.hpp"
#include "window.qt.impl.hpp"

#include "instantiate.hpp"

#include <QWindow>

#include <fmt/core.h>
#include <fmt/xchar.h>
#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    window::window(const options &options) : m_impl(std::make_unique<impl>())
    {
        static QApplication *application;

        static int argc{1};
        static std::vector<char *> argv{strdup("saucer")};

        if (!application)
        {
#ifndef SAUCER_TESTS
            qputenv("QT_LOGGING_RULES", "*=false");
#endif

            auto args = options.chrome_flags;

            if (options.hardware_acceleration)
            {
                args.emplace_back("--enable-oop-rasterization");
                args.emplace_back("--enable-gpu-rasterization");

                args.emplace_back("--use-gl=desktop");
                args.emplace_back("--enable-native-gpu-memory-buffers");
            }

            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", fmt::format("{}", fmt::join(args, " ")));

#ifdef SAUCER_QT5
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

            application = new QApplication(argc, argv.data());
        }

        m_impl->window = new impl::main_window(this);

        //? Fixes QT-Bug where Web-View will not render when background color is transparent.

        auto palette = m_impl->window->palette();
        palette.setColor(QPalette::ColorRole::Window, QColor(255, 255, 255));

        m_impl->window->setPalette(palette);
    }

    window::~window()
    {
        m_impl->window->deleteLater();
    }

    bool window::focused() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return focused(); });
        }

        return m_impl->window->isActiveWindow();
    }

    bool window::minimized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return minimized(); });
        }

        return m_impl->window->isMinimized();
    }

    bool window::maximized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return maximized(); });
        }

        return m_impl->window->isMaximized();
    }

    bool window::resizable() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return resizable(); });
        }

        return m_impl->window->maximumSize() != m_impl->window->minimumSize();
    }

    bool window::decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return decorations(); });
        }

        return !m_impl->window->windowFlags().testFlag(Qt::FramelessWindowHint);
    }

    color window::background() const
    {
        const auto color = m_impl->window->palette().color(QPalette::ColorRole::Window);

        return {
            static_cast<std::uint8_t>(color.red()),
            static_cast<std::uint8_t>(color.green()),
            static_cast<std::uint8_t>(color.blue()),
            static_cast<std::uint8_t>(color.alpha()),
        };
    }

    std::string window::title() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return title(); });
        }

        return m_impl->window->windowTitle().toStdString();
    }

    bool window::always_on_top() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return always_on_top(); });
        }

        return m_impl->window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
    }

    std::pair<int, int> window::size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return size(); });
        }

        return {m_impl->window->width(), m_impl->window->height()};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return max_size(); });
        }

        return {m_impl->window->maximumWidth(), m_impl->window->maximumHeight()};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return min_size(); });
        }

        return {m_impl->window->minimumWidth(), m_impl->window->minimumHeight()};
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

    void window::focus()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return focus(); });
        }

        m_impl->window->activateWindow();
    }

    void window::start_drag()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return start_drag(); });
        }

        m_impl->window->windowHandle()->startSystemMove();
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([edge, this] { return start_resize(edge); });
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
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([enabled, this] { return set_minimized(enabled); });
        }

        auto state = m_impl->window->windowState();

        if (enabled)
        {
            state |= Qt::WindowState::WindowMinimized;
        }
        else
        {
            state &= ~Qt::WindowState::WindowMinimized;
        }

        m_impl->window->setWindowState(state);
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([enabled, this] { return set_maximized(enabled); });
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
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([enabled, this] { return set_resizable(enabled); });
        }

        if (!enabled)
        {
            m_impl->window->setFixedSize(m_impl->window->size());
            return;
        }

        auto max_size = m_impl->max_size.value_or(QSize{QWIDGETSIZE_MAX, QWIDGETSIZE_MAX});
        m_impl->window->setMaximumSize(max_size);

        auto min_size = m_impl->min_size.value_or(QSize{0, 0});
        m_impl->window->setMinimumSize(min_size);
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([enabled, this] { return set_decorations(enabled); });
        }

        m_impl->window->setWindowFlag(Qt::FramelessWindowHint, !enabled);
    }

    void window::set_title(const std::string &title)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([title, this] { return set_title(title); });
        }

        m_impl->window->setWindowTitle(QString::fromStdString(title));
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([enabled, this] { return set_always_on_top(enabled); });
        }

        m_impl->window->setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    }

    void window::set_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([width, height, this] { return set_size(width, height); });
        }

        m_impl->window->resize(width, height);
    }

    void window::set_max_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([width, height, this] { return set_max_size(width, height); });
        }

        m_impl->window->setMaximumSize(width, height);
        m_impl->max_size = m_impl->window->maximumSize();
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([width, height, this] { return set_min_size(width, height); });
        }

        m_impl->window->setMinimumSize(width, height);
        m_impl->min_size = m_impl->window->minimumSize();
    }

    void window::set_background(const color &color)
    {
        auto palette = m_impl->window->palette();

        auto [r, g, b, a] = color;
        palette.setColor(QPalette::ColorRole::Window, {r, g, b, a});

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

    template <window_event Event>
    void window::once(events::type_t<Event> &&callback)
    {
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type_t<Event> &&callback)
    {
        return m_events.at<Event>().add(std::move(callback));
    }

    template <>
    void window::run<true>()
    {
        QApplication::exec();
    }

    template <>
    void window::run<false>()
    {
        QApplication::processEvents();
    }

    INSTANTIATE_EVENTS(window, 6, window_event)
} // namespace saucer
