#include "cocoa.window.impl.hpp"

#include "instantiate.hpp"
#include "cocoa.icon.impl.hpp"

namespace saucer
{
    window::window(const options &) : m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;

        std::call_once(flag,
                       []()
                       {
                           impl::init_objc();

                           static auto *delegate = [[AppDelegate alloc] init];

                           impl::application = app_ptr{[NSApplication sharedApplication],
                                                       [](NSApplication *)
                                                       {
                                                           [NSApp terminate:nil];
                                                           delegate = nil;
                                                       }};

                           [NSApp setDelegate:delegate];
                           [NSApp activateIgnoringOtherApps:YES];
                           [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                       });

        if (!impl::application) [[unlikely]]
        {
            throw std::runtime_error{"Construction outside of the main-thread is not permitted"};
        }

        static constexpr auto mask = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                                     NSWindowStyleMaskTitled | NSWindowStyleMaskResizable;

        m_impl->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                     styleMask:mask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];

        m_impl->delegate = [[WindowDelegate alloc] initWithParent:this];

        [m_impl->window setDelegate:m_impl->delegate];
        [m_impl->window center];
    }

    window::~window()
    {
        m_impl->window = nil;
    }

    void window::dispatch(callback_t callback) const // NOLINT
    {
        auto work = [](callback_t *data)
        {
            auto callback = std::unique_ptr<callback_t>{data};
            std::invoke(*callback);
        };

        dispatch_async_f(dispatch_get_main_queue(), new callback_t{std::move(callback)},
                         reinterpret_cast<dispatch_function_t>(+work));
    }

    bool window::focused() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return focused(); }).get();
        }

        return m_impl->window.isKeyWindow;
    }

    bool window::minimized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return minimized(); }).get();
        }

        return m_impl->window.isMiniaturized;
    }

    bool window::maximized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return maximized(); }).get();
        }

        return m_impl->window.isZoomed;
    }

    bool window::resizable() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return resizable(); }).get();
        }

        return m_impl->window.styleMask & NSWindowStyleMaskResizable;
    }

    bool window::decorations() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return decorations(); }).get();
        }

        return m_impl->window.styleMask != NSWindowStyleMaskBorderless;
    }

    bool window::always_on_top() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return always_on_top(); }).get();
        }

        return m_impl->window.level == kCGMaximumWindowLevelKey;
    }

    std::string window::title() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return title(); }).get();
        }

        return m_impl->window.title.UTF8String;
    }

    std::pair<int, int> window::size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return size(); }).get();
        }

        const auto [width, height] = m_impl->window.frame.size;

        return {width, height};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return max_size(); }).get();
        }

        const auto [width, height] = m_impl->window.maxSize;

        return {width, height};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return min_size(); }).get();
        }

        const auto [width, height] = m_impl->window.minSize;

        return {width, height};
    }

    void window::hide()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return hide(); }).get();
        }

        [m_impl->window orderOut:nil];
    }

    void window::show()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return show(); }).get();
        }

        [m_impl->window makeKeyAndOrderFront:nil];
    }

    void window::close()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return close(); }).get();
        }

        [m_impl->window close];
    }

    void window::focus()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return focus(); }).get();
        }

        [m_impl->window makeKeyWindow];
    }

    void window::start_drag()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return start_drag(); }).get();
        }

        [m_impl->window performWindowDragWithEvent:NSApp.currentEvent];
    }

    void window::start_resize(window_edge edge)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return start_drag(); }).get();
        }

        m_impl->edge.emplace(edge);
    }

    void window::set_minimized(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_minimized(enabled); }).get();
        }

        [m_impl->window setIsMiniaturized:static_cast<BOOL>(enabled)];
    }

    void window::set_maximized(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_maximized(enabled); }).get();
        }

        [m_impl->window setIsZoomed:static_cast<BOOL>(enabled)];
    }

    void window::set_resizable(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(enabled); }).get();
        }

        static constexpr auto flag = NSWindowStyleMaskResizable;
        auto mask                  = m_impl->window.styleMask;

        if (!enabled)
        {
            mask &= ~flag;
        }
        else
        {
            mask |= flag;
        }

        [m_impl->window setStyleMask:mask];
    }

    void window::set_decorations(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(enabled); }).get();
        }

        const auto mask = m_impl->window.styleMask;

        if (mask != NSWindowStyleMaskBorderless)
        {
            m_impl->prev_mask = mask;
        }

        [m_impl->window setStyleMask:enabled ? m_impl->prev_mask : NSWindowStyleMaskBorderless];
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_always_on_top(enabled); }).get();
        }

        m_impl->window.level = enabled ? kCGMaximumWindowLevelKey : kCGNormalWindowLevelKey;
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!impl::is_thread_safe())
        {
            return dispatch([this, icon] { return set_icon(icon); }).get();
        }

        auto *const view = [NSImageView imageViewWithImage:icon.m_impl->icon];
        auto *const tile = m_impl->application.get().dockTile;

        [tile setContentView:view];
        [tile display];
    }

    void window::set_title(const std::string &title)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, title] { return set_title(title); }).get();
        }

        [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void window::set_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height] { return set_size(width, height); }).get();
        }

        auto frame = m_impl->window.frame;
        frame.size = {static_cast<float>(width), static_cast<float>(height)};

        [m_impl->window setFrame:frame display:YES animate:YES];
    }

    void window::set_max_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height] { return set_max_size(width, height); }).get();
        }

        [m_impl->window setMaxSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::set_min_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height] { return set_min_size(width, height); }).get();
        }

        [m_impl->window setMinSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::clear(window_event event)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, event] { return clear(event); }).get();
        }

        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, event, id] { return remove(event, id); }).get();
        }

        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::type<Event> callback)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable
                            { return once<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type<Event> callback)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return on<Event>(std::move(callback)); })
                .get();
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    template <>
    void window::run<true>()
    {
        [NSApp run];
    }

    template <>
    void window::run<false>()
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

    INSTANTIATE_EVENTS(window, 6, window_event)
} // namespace saucer
