#include "cocoa.app.impl.hpp"

#include <algorithm>

namespace saucer
{
    using impl = application::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        platform = std::make_unique<native>();

        platform->application                = [NSApplication sharedApplication];
        platform->id                         = opts.id.value();
        platform->quit_on_last_window_closed = opts.quit_on_last_window_closed;

        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        native::init_menu();

        return {};
    };

    impl::~impl() = default;

    std::vector<screen> impl::screens() const // NOLINT(*-static)
    {
        const auto guard    = utils::autorelease_guard{};
        auto *const screens = [NSScreen screens];

        std::vector<screen> rtn{};
        rtn.reserve(screens.count);

        for (NSScreen *entry : screens)
        {
            rtn.emplace_back(native::convert(entry));
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
                           (*callback)();
                       });
    }

    int impl::run(application *self, callback_t callback)
    {
        const utils::autorelease_guard guard{};

        auto promise = coco::promise<void>{};
        finish       = promise.get_future();

        self->post([self, &callback] { callback(self); });
        [NSApp run];

        promise.set_value();

        return 0;
    }

    void impl::quit() // NOLINT(*-static)
    {
        [NSApp stop:nil];
    }
} // namespace saucer
