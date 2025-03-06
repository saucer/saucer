#include "qt.app.impl.hpp"

#include <QThread>

namespace saucer
{
    application::application(const options &opts) : extensible(this), m_pool(opts.threads), m_impl(std::make_unique<impl>())
    {
        m_impl->id = opts.id.value();

        m_impl->argv = {m_impl->id.data()};
        m_impl->argc = static_cast<int>(m_impl->argv.size());

        if (opts.argc && opts.argv)
        {
            m_impl->argc = opts.argc.value();
            m_impl->argv = {opts.argv.value(), opts.argv.value() + opts.argc.value()};
        }

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

    std::vector<screen> application::screens() const
    {
        if (!thread_safe())
        {
            return dispatch([this] { return screens(); });
        }

        const auto screens = m_impl->application->screens();

        std::vector<screen> rtn;
        rtn.reserve(screens.size());

        for (const auto &screen : screens)
        {
            rtn.emplace_back(impl::convert(screen));
        }

        return rtn;
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
