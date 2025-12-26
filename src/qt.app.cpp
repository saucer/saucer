#include "qt.app.impl.hpp"

namespace saucer
{
    using impl = application::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        platform = std::make_unique<native>();

        auto make_args = [](auto &&argv)
        {
            return std::vector<char *>{argv, argv + native::argc};
        };

        native::id   = opts.id.value();
        native::argc = opts.argc.value_or(1);
        native::argv = opts.argv.transform(make_args).value_or({native::id.data()});

        platform->application = std::make_unique<QApplication>(native::argc, native::argv.data());
        platform->application->setQuitOnLastWindowClosed(opts.quit_on_last_window_closed);

        return {};
    }

    impl::~impl() = default;

    std::vector<screen> impl::screens() const
    {
        const auto screens = platform->application->screens();

        std::vector<screen> rtn;
        rtn.reserve(screens.size());

        for (const auto &screen : screens)
        {
            rtn.emplace_back(native::convert(screen));
        }

        return rtn;
    }

    void application::post(post_callback_t callback) const
    {
        auto *const event = new safe_event{std::move(callback)};
        QApplication::postEvent(m_impl->platform->application.get(), event);
    }

    int impl::run(application *self, callback_t callback)
    {
        auto promise = coco::promise<void>{};
        finish       = promise.get_future();

        self->post([self, &callback] { callback(self); });
        const auto rtn = QApplication::exec();
        promise.set_value();

        return rtn;
    }

    void impl::quit() // NOLINT(*-static)
    {
        QApplication::quit();
    }
} // namespace saucer
