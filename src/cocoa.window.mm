#include "cocoa.window.impl.hpp"

#include "instantiate.hpp"

#include "cocoa.app.impl.hpp"
#include "cocoa.icon.impl.hpp"

#include <cassert>
#include <rebind/enum.hpp>

namespace saucer
{
    window::window(const preferences &prefs) : m_impl(std::make_unique<impl>()), m_parent(prefs.application.value())
    {
        assert(m_parent->thread_safe() && "Construction outside of the main-thread is not permitted");

        static std::once_flag flag;
        std::call_once(flag, [] { impl::init_objc(); });

        static constexpr auto mask = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled |
                                     NSWindowStyleMaskResizable;

        const autorelease_guard guard{};

        m_impl->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                     styleMask:mask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];

        m_impl->delegate = [[WindowDelegate alloc] initWithParent:this];

        [m_impl->window setDelegate:m_impl->delegate.get()];
        [m_impl->window center];
    }

    window::~window()
    {
        const autorelease_guard guard{};

        for (const auto &event : rebind::enum_fields<window_event>)
        {
            m_events.clear(event.value);
        }

        close();
        [m_impl->window close];
    }

    bool window::focused() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return focused(); });
        }

        return m_impl->window.isKeyWindow;
    }

    bool window::minimized() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return minimized(); });
        }

        return m_impl->window.isMiniaturized;
    }

    bool window::maximized() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return maximized(); });
        }

        return m_impl->window.isZoomed;
    }

    bool window::resizable() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return resizable(); });
        }

        return m_impl->window.styleMask & NSWindowStyleMaskResizable;
    }

    bool window::decorations() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return decorations(); });
        }

        return m_impl->window.styleMask != NSWindowStyleMaskBorderless;
    }

    bool window::always_on_top() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return always_on_top(); });
        }

        return m_impl->window.level == kCGMaximumWindowLevelKey;
    }

    std::string window::title() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return title(); });
        }

        return m_impl->window.title.UTF8String;
    }

    std::pair<int, int> window::size() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return size(); });
        }

        const auto [width, height] = m_impl->window.frame.size;
        return {width, height};
    }

    std::pair<int, int> window::max_size() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return max_size(); });
        }

        const auto [width, height] = m_impl->window.maxSize;
        return {width, height};
    }

    std::pair<int, int> window::min_size() const
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return min_size(); });
        }

        const auto [width, height] = m_impl->window.minSize;
        return {width, height};
    }

    void window::hide()
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return hide(); });
        }

        [m_impl->window orderOut:nil];
    }

    void window::show()
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return show(); });
        }

        m_parent->native()->instances[m_impl->window] = true;
        [m_impl->window makeKeyAndOrderFront:nil];
    }

    void window::close()
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return close(); });
        }

        [m_impl->delegate.get() windowShouldClose:m_impl->window];
    }

    void window::focus()
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return focus(); });
        }

        [m_impl->window makeKeyWindow];
    }

    void window::start_drag()
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return start_drag(); });
        }

        [m_impl->window performWindowDragWithEvent:NSApp.currentEvent];
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return start_drag(); });
        }

        m_impl->edge.emplace(edge);
    }

    void window::set_minimized(bool enabled)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_minimized(enabled); });
        }

        [m_impl->window setIsMiniaturized:static_cast<BOOL>(enabled)];
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_maximized(enabled); });
        }

        [m_impl->window setIsZoomed:static_cast<BOOL>(enabled)];
    }

    void window::set_resizable(bool enabled)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(enabled); });
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
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(enabled); });
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
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, enabled] { return set_always_on_top(enabled); });
        }

        m_impl->window.level = enabled ? kCGMaximumWindowLevelKey : kCGNormalWindowLevelKey;
    }

    void window::set_icon(const icon &icon)
    {
        const autorelease_guard guard{};

        if (icon.empty())
        {
            return;
        }

        if (!m_parent->thread_safe())
        {
            return dispatch([this, icon] { return set_icon(icon); });
        }

        auto *const view = [NSImageView imageViewWithImage:icon.m_impl->icon.get()];
        auto *const tile = m_parent->native()->application.dockTile;

        [tile setContentView:view];
        [tile display];
    }

    void window::set_title(const std::string &title)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, title] { return set_title(title); });
        }

        [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void window::set_size(int width, int height)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_size(width, height); });
        }

        auto frame = m_impl->window.frame;
        frame.size = {static_cast<float>(width), static_cast<float>(height)};

        [m_impl->window setFrame:frame display:YES animate:YES];
    }

    void window::set_max_size(int width, int height)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_max_size(width, height); });
        }

        [m_impl->window setMaxSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::set_min_size(int width, int height)
    {
        const autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return dispatch([this, width, height] { return set_min_size(width, height); });
        }

        [m_impl->window setMinSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::clear(window_event event)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, event] { return clear(event); });
        }

        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, event, id] { return remove(event, id); });
        }

        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, callback = std::move(callback)] mutable { return once<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type<Event> callback)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, callback = std::move(callback)]() mutable //
                            { return on<Event>(std::move(callback)); });
        }

        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    INSTANTIATE_EVENTS(window, 7, window_event)
} // namespace saucer
