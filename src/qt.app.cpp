#include "qt.app.impl.hpp"

#include <QThread>

namespace saucer
{
    application::application(impl data) : extensible(this), m_impl(std::make_unique<impl>(std::move(data))) {}

    application::~application() = default;

    bool application::thread_safe() const
    {
        return m_impl->application->thread() == QThread::currentThread();
    }

    coco::future<void> application::finish()
    {
        return std::move(m_impl->future);
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

    void application::post(post_callback_t callback) const
    {
        auto *const event = new safe_event{std::move(callback)};
        QApplication::postEvent(m_impl->application.get(), event);
    }

    int application::run(callback_t callback)
    {
        static bool once{false};

        if (once)
        {
            assert(false && "saucer::application::run() may only be called once");
            return -1;
        }

        auto promise = coco::promise<void>{};

        once           = true;
        m_impl->future = promise.get_future();

        post([this, callback = std::move(callback)] mutable { std::invoke(callback, this); });
        const auto rtn = QApplication::exec();

        promise.set_value();

        return rtn;
    }

    void application::quit() // NOLINT(*-static)
    {
        QApplication::quit();
    }

    std::optional<application> application::create(const options &opts)
    {
        static bool once{false};

        if (once)
        {
            assert(false && "saucer::application may only be created once");
            return std::nullopt;
        }

        once = true;

#ifdef SAUCER_QT5
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        impl rtn;

        auto make_args = [&](auto &&argv)
        {
            return std::vector<char *>{argv, argv + rtn.argc};
        };

        rtn.id = opts.id.value();

        impl::argc = opts.argc.value_or(1);
        impl::argv = opts.argv.transform(make_args).value_or({rtn.id.data()});

        rtn.application = std::make_unique<QApplication>(impl::argc, impl::argv.data());

        return rtn;
    }
} // namespace saucer
