#include "cocoa.app.impl.hpp"

namespace saucer
{
    application::application(impl data) : extensible(this), m_impl(std::make_unique<impl>(std::move(data)))
    {
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        impl::init_menu();
    }

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
        const utils::autorelease_guard guard{};

        if (!thread_safe())
        {
            return dispatch([this] { return screens(); });
        }

        auto *const screens = [NSScreen screens];

        std::vector<screen> rtn{};
        rtn.reserve(screens.count);

        for (NSScreen *entry : screens)
        {
            rtn.emplace_back(impl::convert(entry));
        }

        return rtn;
    }

    void application::post(post_callback_t callback) const // NOLINT(*-static)
    {
        auto *const queue = dispatch_get_main_queue();
        auto *const ptr   = new post_callback_t{std::move(callback)};

        dispatch_async(queue,
                       [ptr]
                       {
                           const auto guard = utils::autorelease_guard{};
                           auto callback    = std::unique_ptr<post_callback_t>{ptr};
                           std::invoke(*callback);
                       });
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

        const auto guard = utils::autorelease_guard{};
        auto promise     = coco::promise<void>{};

        once           = true;
        m_impl->future = promise.get_future();

        post([this, callback = std::move(callback)]() mutable { std::invoke(callback, this); });
        [NSApp run];

        promise.set_value();

        return 0;
    }

    void application::quit() // NOLINT(*-static)
    {
        [NSApp stop:nil];
    }

    std::optional<application> application::create(const options &)
    {
        static bool once{false};

        if (once)
        {
            assert(false && "saucer::application may only be created once");
            return std::nullopt;
        }

        once = true;

        return impl{
            .application = [NSApplication sharedApplication],
            .thread      = std::this_thread::get_id(),
        };
    }
} // namespace saucer
