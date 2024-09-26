#include "qt.app.impl.hpp"

#include <QThread>

namespace saucer
{
    application::application(const options &options) : m_impl(std::make_unique<impl>())
    {
        m_impl->id = options.id.value();

        m_impl->argv = {m_impl->id.data()};
        m_impl->argc = static_cast<int>(m_impl->argv.size());

#ifndef SAUCER_TESTS
        qputenv("QT_LOGGING_RULES", "*=false");
#endif

#ifdef SAUCER_QT5
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        m_impl->application = std::make_unique<QApplication>(m_impl->argc, m_impl->argv.data());
    }

    application::~application() = default;

    bool application::thread_safe() const
    {
        return m_impl->application->thread() == QThread::currentThread();
    }

    void application::post(callback_t callback) const
    {
        auto *const event = new safe_event{std::move(callback)};
        QApplication::postEvent(m_impl->application.get(), event);
    }

    template <>
    void application::run<true>() const // NOLINT(*-static)
    {
        QApplication::exec();
    }

    template <>
    void application::run<false>() const // NOLINT(*-static)
    {
        QApplication::processEvents();
    }

    void application::quit() // NOLINT(*-static)
    {
        QApplication::quit();
    }
} // namespace saucer
