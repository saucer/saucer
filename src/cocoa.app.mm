#include "cocoa.app.impl.hpp"

namespace saucer
{
    application::application(const options &) : m_impl(std::make_unique<impl>())
    {
        m_impl->thread      = std::this_thread::get_id();
        m_impl->application = [NSApplication sharedApplication];

        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        impl::init_menu();
    }

    application::~application()
    {
        m_impl->application = nil;
        [NSApp terminate:nil];
    }

    void application::dispatch(callback_t callback) const // NOLINT(*-static)
    {
        auto func = [](callback_t *data)
        {
            auto callback = std::unique_ptr<callback_t>{data};
            std::invoke(*callback);
        };

        auto *const queue = dispatch_get_main_queue();
        dispatch_async_f(queue, new callback_t{std::move(callback)}, reinterpret_cast<dispatch_function_t>(+func));
    }

    bool application::thread_safe() const
    {
        return m_impl->thread == std::this_thread::get_id();
    }

    template <>
    void application::run<true>() const // NOLINT(*-static)
    {
        [NSApp run];
    }

    template <>
    void application::run<false>() const // NOLINT(*-static)
    {
        auto *const event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                               untilDate:[NSDate now]
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES];

        if (!event)
        {
            return;
        }

        [NSApp sendEvent:event];
    }

    void application::quit() // NOLINT(*-static)
    {
        [NSApp stop:nil];
    }
} // namespace saucer