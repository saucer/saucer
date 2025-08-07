#include "gtk.app.impl.hpp"

#include <format>

namespace saucer
{
    using impl = application::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        platform = std::make_unique<native>();

        auto id = opts.id.value();

        if (!g_application_id_is_valid(id.c_str()))
        {
            id = std::format("app.saucer.{}", native::fix_id(id));
        }

        platform->application = adw_application_new(id.c_str(), G_APPLICATION_DEFAULT_FLAGS);

        platform->argc                       = opts.argc.value_or(0);
        platform->argv                       = opts.argv.value_or(nullptr);
        platform->quit_on_last_window_closed = opts.quit_on_last_window_closed;

        if (!opts.quit_on_last_window_closed)
        {
            // Required so that application does not quit immediately if there are no windows
            g_application_hold(G_APPLICATION(platform->application.get()));
        }

        return {};
    }

    // We don't need to call `g_application_release` anymore as we're explicitly calling quit
    impl::~impl() = default;

    std::vector<screen> impl::screens() const // NOLINT(*-static)
    {
        auto *const display  = gdk_display_get_default();
        auto *const monitors = gdk_display_get_monitors(display);
        const auto size      = g_list_model_get_n_items(monitors);

        std::vector<screen> rtn{};
        rtn.reserve(size);

        for (auto i = 0uz; size > i; ++i)
        {
            auto *const current = reinterpret_cast<GdkMonitor *>(g_list_model_get_item(monitors, i));
            rtn.emplace_back(native::convert(current));
        }

        return rtn;
    }

    void application::post(post_callback_t callback) const // NOLINT(*-static)
    {
        auto once = [](post_callback_t *data)
        {
            auto callback = std::unique_ptr<post_callback_t>{data};
            std::invoke(*callback);
        };

        g_idle_add_once(reinterpret_cast<GSourceOnceFunc>(+once), new post_callback_t{std::move(callback)});
    }

    int impl::run(application *self, callback_t callback)
    {
        auto promise = coco::promise<void>{};
        auto data    = std::make_tuple(std::move(callback), self);

        finish = promise.get_future();

        auto activate = [](GtkApplication *, decltype(data) *data)
        {
            static std::once_flag flag;
            auto &[callback, self] = *data;
            std::call_once(flag, callback, self);
        };

        utils::connect(platform->application.get(), "activate", +activate, &data);
        const auto rtn = g_application_run(G_APPLICATION(platform->application.get()), platform->argc, platform->argv);

        promise.set_value();

        return rtn;
    }

    void impl::quit() // NOLINT(*-const)
    {
        g_application_quit(G_APPLICATION(platform->application.get()));
    }
} // namespace saucer
