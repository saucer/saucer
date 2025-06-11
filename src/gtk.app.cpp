#include "gtk.app.impl.hpp"

#include <format>
#include <cassert>
#include <algorithm>

namespace saucer
{
    application::application(impl data) : extensible(this), m_impl(std::make_unique<impl>(std::move(data))) {}

    application::~application() = default;

    bool application::thread_safe() const
    {
        return m_impl->thread == std::this_thread::get_id();
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

        auto *const display  = gdk_display_get_default();
        auto *const monitors = gdk_display_get_monitors(display);
        const auto size      = g_list_model_get_n_items(monitors);

        std::vector<screen> rtn{};
        rtn.reserve(size);

        for (auto i = 0uz; size > i; ++i)
        {
            auto *const current = reinterpret_cast<GdkMonitor *>(g_list_model_get_item(monitors, i));
            rtn.emplace_back(impl::convert(current));
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

    int application::run(callback_t callback)
    {
        static bool once{false};

        if (!thread_safe())
        {
            assert(false && "saucer::application::run() may only be called from the thread it was created in");
            return -1;
        }

        if (once)
        {
            assert(false && "saucer::application::run() may only be called once");
            return -1;
        }

        auto promise = coco::promise<void>{};
        auto data    = std::make_tuple(std::move(callback), this);

        once           = true;
        m_impl->future = promise.get_future();

        auto activate = [](GtkApplication *, void *raw)
        {
            static std::once_flag flag;
            auto &[callback, self] = *reinterpret_cast<decltype(data) *>(raw);
            std::call_once(flag, callback, self);
        };

        g_signal_connect(m_impl->application.get(), "activate", G_CALLBACK(+activate), &data);
        const auto rtn = g_application_run(G_APPLICATION(m_impl->application.get()), m_impl->argc, m_impl->argv);

        promise.set_value();

        return rtn;
    }

    void application::quit()
    {
        using traits = modules::traits<application>;

        if (!thread_safe())
        {
            return dispatch([this] { return quit(); });
        }

        if (std::ranges::any_of(modules(), [](auto &module) { return module.template invoke<traits::on_quit>(); }))
        {
            return;
        }

        g_application_quit(G_APPLICATION(m_impl->application.get()));
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

        const auto id = g_application_id_is_valid(opts.id->c_str()) //
                            ? opts.id.value()
                            : std::format("app.saucer.{}", impl::fix_id(opts.id.value()));

        return impl{
            .application                = adw_application_new(id.c_str(), G_APPLICATION_DEFAULT_FLAGS),
            .argc                       = opts.argc.value_or(0),
            .argv                       = opts.argv.value_or(nullptr),
            .thread                     = std::this_thread::get_id(),
            .quit_on_last_window_closed = opts.quit_on_last_window_closed,
        };
    }
} // namespace saucer
