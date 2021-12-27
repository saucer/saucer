#include <QApplication>
#include <QCloseEvent>
#include <QMainWindow>
#include <QThread>
#include <optional>
#include <window.hpp>

namespace saucer
{
    class saucer_main_window : public QMainWindow
    {
      private:
        std::function<bool()> &m_close_callback;
        std::function<void(std::size_t, std::size_t)> &m_resize_callback;

      public:
        saucer_main_window(std::function<bool()> &close_callback, std::function<void(std::size_t, std::size_t)> &resize_callback)
            : m_close_callback(close_callback), m_resize_callback(resize_callback)
        {
        }

      protected:
        void resizeEvent(QResizeEvent *event) override
        {
            if (m_resize_callback)
            {
                m_resize_callback(width(), height());
            }

            QMainWindow::resizeEvent(event);
        }

        void closeEvent(QCloseEvent *event) override
        {
            if (m_close_callback)
            {
                if (m_close_callback())
                {
                    event->ignore();
                }
                else
                {
                    QMainWindow::closeEvent(event);
                }
            }
        }
    };

    struct window::impl
    {
        std::unique_ptr<QMainWindow> window;
        std::optional<QSize> max_size;
        std::optional<QSize> min_size;

      public:
        static QApplication *g_application;
        bool is_thread_safe() const;
    };
    QApplication *window::impl::g_application;

    bool window::impl::is_thread_safe() const
    {
        return window->thread() == QThread::currentThread();
    }

    class safe_call : public QEvent
    {
        std::function<void()> m_func;

      public:
        safe_call(std::function<void()> &&func) : QEvent(QEvent::None), m_func(func) {}
        ~safe_call() override
        {
            m_func();
        }
    };

    window::~window() = default;
    window::window() : m_impl(std::make_unique<impl>())
    {
        qputenv("QT_LOGGING_RULES", "*=false");

        if (!m_impl->g_application)
        {
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

            static int argc = 0;
            m_impl->g_application = new QApplication(argc, nullptr); //? The QApplication has to exist until the program exits
        }

        m_impl->window = std::make_unique<saucer_main_window>(m_close_callback, m_resize_callback);

        auto palette = m_impl->window->palette();
        palette.setColor(QPalette::ColorRole::Window, QColor(255, 255, 255));

        m_impl->window->setPalette(palette);
    }

    void window::run()
    {
        QApplication::exec();
    }

    void window::set_title(const std::string &title)
    {
        m_impl->window->setWindowTitle(QString::fromStdString(title));
    }

    void window::set_resizeable(bool enabled)
    {
        if (!enabled)
        {
            auto current_size = m_impl->window->size();
            m_impl->max_size = m_impl->window->maximumSize();
            m_impl->min_size = m_impl->window->maximumSize();

            m_impl->window->setMaximumSize(current_size);
            m_impl->window->setMinimumSize(current_size);
        }
        else
        {
            if (m_impl->max_size)
            {
                m_impl->window->setMaximumSize(*m_impl->max_size);
                m_impl->max_size = std::nullopt;
            }

            if (m_impl->min_size)
            {
                m_impl->window->setMinimumSize(*m_impl->min_size);
                m_impl->min_size = std::nullopt;
            }
        }
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { set_decorations(enabled); }));
            return;
        }

        m_impl->window->setWindowFlag(Qt::FramelessWindowHint, !enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { set_always_on_top(enabled); }));
            return;
        }

        m_impl->window->setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    }

    void window::set_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { set_size(width, height); }));
            return;
        }

        m_impl->window->resize(static_cast<int>(width), static_cast<int>(height));
    }

    void window::set_max_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { set_max_size(width, height); }));
            return;
        }

        m_impl->max_size = {static_cast<int>(width), static_cast<int>(height)};
        m_impl->window->setMaximumSize(*m_impl->max_size);
    }

    void window::set_min_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { set_min_size(width, height); }));
            return;
        }

        m_impl->min_size = {static_cast<int>(width), static_cast<int>(height)};
        m_impl->window->setMinimumSize(*m_impl->min_size);
    }

    std::pair<std::size_t, std::size_t> window::get_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            std::promise<std::pair<std::size_t, std::size_t>> result;
            QApplication::postEvent(m_impl->window.get(), new safe_call([&]() { result.set_value(get_size()); }));

            return result.get_future().get();
        }

        const auto size = m_impl->window->size();
        return std::make_pair(size.width(), size.height());
    }

    std::pair<std::size_t, std::size_t> window::get_min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            std::promise<std::pair<std::size_t, std::size_t>> result;
            QApplication::postEvent(m_impl->window.get(), new safe_call([&]() { result.set_value(get_min_size()); }));

            return result.get_future().get();
        }

        const auto size = m_impl->window->minimumSize();
        return std::make_pair(size.width(), size.height());
    }

    std::pair<std::size_t, std::size_t> window::get_max_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            std::promise<std::pair<std::size_t, std::size_t>> result;
            QApplication::postEvent(m_impl->window.get(), new safe_call([&]() { result.set_value(get_max_size()); }));

            return result.get_future().get();
        }

        const auto size = m_impl->window->maximumSize();
        return std::make_pair(size.width(), size.height());
    }

    std::string window::get_title() const
    {
        return m_impl->window->windowTitle().toStdString();
    }

    bool window::get_always_on_top() const
    {
        if (!m_impl->is_thread_safe())
        {
            std::promise<bool> result;
            QApplication::postEvent(m_impl->window.get(), new safe_call([&]() { result.set_value(get_always_on_top()); }));

            return result.get_future().get();
        }

        return (m_impl->window->windowFlags() & Qt::WindowStaysOnTopHint);
    }

    bool window::get_decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            std::promise<bool> result;
            QApplication::postEvent(m_impl->window.get(), new safe_call([&]() { result.set_value(get_decorations()); }));

            return result.get_future().get();
        }

        return !(m_impl->window->windowFlags() & Qt::FramelessWindowHint);
    }

    void window::hide()
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { hide(); }));
            return;
        }

        m_impl->window->hide();
    }

    void window::show()
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { show(); }));
            return;
        }

        m_impl->window->show();
    }

    void window::on_resize(const resize_callback_t &callback)
    {
        m_resize_callback = callback;
    }

    void window::on_close(const close_callback_t &callback)
    {
        m_close_callback = callback;
    }

    bool window::get_resizeable() const
    {
        return !m_impl->max_size && !m_impl->min_size;
    }

    void window::exit()
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->window.get(), new safe_call([=]() { exit(); }));
            return;
        }

        auto old_callback = m_close_callback;
        m_close_callback = nullptr;

        m_impl->window->close();
        m_close_callback = old_callback;
    }
} // namespace saucer