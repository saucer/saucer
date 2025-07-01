#include "cocoa.window.impl.hpp"

#include "instantiate.hpp"

#include "cocoa.app.impl.hpp"
#include "cocoa.icon.impl.hpp"

#include <cassert>

#include <rebind/enum.hpp>

namespace saucer
{
    window::window(application *parent) : m_parent(parent), m_impl(std::make_unique<impl>())
    {
        assert(m_parent->thread_safe() && "Construction outside of the main-thread is not permitted");

        const utils::autorelease_guard guard{};

        m_impl->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                     styleMask:mask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];

        set_resizable(true);

        m_impl->delegate = [[WindowDelegate alloc] initWithParent:this events:&m_events];

        [m_impl->window setDelegate:m_impl->delegate.get()];
        [m_impl->window center];
    }

    window::~window()
    {
        const utils::autorelease_guard guard{};

        for (const auto &event : rebind::enum_values<window_event>)
        {
            m_events.clear(event);
        }

        // We hide-on-close, so we call trigger two different close calls to properly quit.

        close();
        [m_impl->window close];
    }

    template <window_event Event>
    void window::setup()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return setup<Event>(); });
        }

        m_impl->setup<Event>(this);
    }

    bool window::visible() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return visible(); });
        }

        return m_impl->window.isVisible;
    }

    bool window::focused() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focused(); });
        }

        return m_impl->window.isKeyWindow;
    }

    bool window::minimized() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return minimized(); });
        }

        return m_impl->window.isMiniaturized;
    }

    bool window::maximized() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return maximized(); });
        }

        return m_impl->window.isZoomed;
    }

    bool window::resizable() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return resizable(); });
        }

        return m_impl->window.styleMask & NSWindowStyleMaskResizable;
    }

    bool window::always_on_top() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return always_on_top(); });
        }

        return m_impl->window.level == kCGMaximumWindowLevelKey;
    }

    bool window::click_through() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return click_through(); });
        }

        return m_impl->window.ignoresMouseEvents;
    }

    std::string window::title() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return title(); });
        }

        return m_impl->window.title.UTF8String;
    }

    window_decoration window::decoration() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return decoration(); });
        }

        if (m_impl->window.styleMask == NSWindowStyleMaskBorderless)
        {
            return window_decoration::none;
        }

        if (m_impl->window.styleMask & NSWindowStyleMaskFullSizeContentView)
        {
            return window_decoration::partial;
        }

        return window_decoration::full;
    }

    std::pair<int, int> window::size() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return size(); });
        }

        const auto [width, height] = m_impl->window.frame.size;
        return {width, height};
    }

    std::pair<int, int> window::max_size() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return max_size(); });
        }

        const auto [width, height] = m_impl->window.maxSize;
        return {width, height};
    }

    std::pair<int, int> window::min_size() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return min_size(); });
        }

        const auto [width, height] = m_impl->window.minSize;
        return {width, height};
    }

    std::pair<int, int> window::position() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return position(); });
        }

        const auto [x, y] = m_impl->window.frame.origin;
        return {x, y};
    }

    std::optional<saucer::screen> window::screen() const
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return screen(); });
        }

        auto *const screen = m_impl->window.screen;

        if (!screen)
        {
            return std::nullopt;
        }

        return application::impl::convert(screen);
    }

    void window::hide()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return hide(); });
        }

        [m_impl->window orderOut:nil];
    }

    void window::show()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return show(); });
        }

        m_parent->native<false>()->instances[m_impl->window] = true;
        [m_impl->window makeKeyAndOrderFront:nil];
    }

    void window::close()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return close(); });
        }

        [m_impl->delegate.get() windowShouldClose:m_impl->window];
    }

    void window::focus()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focus(); });
        }

        [m_impl->window makeKeyWindow];
    }

    void window::start_drag()
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return start_drag(); });
        }

        [m_impl->window performWindowDragWithEvent:NSApp.currentEvent];
    }

    void window::start_resize(window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return start_drag(); });
        }

        m_impl->edge.emplace(edge);
    }

    void window::set_minimized(bool enabled)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_minimized(enabled); });
        }

        [m_impl->window setIsMiniaturized:static_cast<BOOL>(enabled)];
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_maximized(enabled); });
        }

        [m_impl->window setIsZoomed:static_cast<BOOL>(enabled)];
    }

    void window::set_resizable(bool enabled)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_resizable(enabled); });
        }

        static constexpr auto flag = NSWindowStyleMaskResizable;

        if (!enabled)
        {
            m_impl->masks &= ~flag;
        }
        else
        {
            m_impl->masks |= flag;
        }

        [m_impl->window setStyleMask:mask | m_impl->masks];
    }

    void window::set_always_on_top(bool enabled)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_always_on_top(enabled); });
        }

        m_impl->window.level = enabled ? kCGMaximumWindowLevelKey : kCGNormalWindowLevelKey;
    }

    void window::set_click_through(bool enabled)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { return set_click_through(enabled); });
        }

        m_impl->window.ignoresMouseEvents = static_cast<BOOL>(enabled);
    }

    void window::set_icon(const icon &icon)
    {
        const utils::autorelease_guard guard{};

        if (icon.empty())
        {
            return;
        }

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, icon] { return set_icon(icon); });
        }

        auto *const view = [NSImageView imageViewWithImage:icon.native<false>()->icon.get()];
        auto *const tile = m_parent->native<false>()->application.dockTile;

        [tile setContentView:view];
        [tile display];
    }

    void window::set_title(const std::string &title)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, title] { return set_title(title); });
        }

        [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void window::set_decoration(window_decoration decoration)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, decoration] { return set_decoration(decoration); });
        }

        if (decoration == window_decoration::none)
        {
            [m_impl->window setStyleMask:NSWindowStyleMaskBorderless];
            return;
        }

        static constexpr auto flag = NSWindowStyleMaskFullSizeContentView;
        const auto hidden          = static_cast<BOOL>(decoration == window_decoration::partial);

        if (hidden)
        {
            m_impl->masks |= flag;
        }
        else
        {
            m_impl->masks &= ~flag;
        }

        [m_impl->window setStyleMask:mask | m_impl->masks];

        [m_impl->window standardWindowButton:NSWindowZoomButton].hidden        = hidden;
        [m_impl->window standardWindowButton:NSWindowCloseButton].hidden       = hidden;
        [m_impl->window standardWindowButton:NSWindowMiniaturizeButton].hidden = hidden;

        m_impl->window.titlebarAppearsTransparent = hidden;
        m_impl->window.titleVisibility            = hidden ? NSWindowTitleHidden : NSWindowTitleVisible;
    }

    void window::set_size(int width, int height)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_size(width, height); });
        }

        auto frame = m_impl->window.frame;
        frame.size = {static_cast<float>(width), static_cast<float>(height)};

        [m_impl->window setFrame:frame display:YES animate:YES];
    }

    void window::set_max_size(int width, int height)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_max_size(width, height); });
        }

        [m_impl->window setMaxSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::set_min_size(int width, int height)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { return set_min_size(width, height); });
        }

        [m_impl->window setMinSize:{static_cast<float>(width), static_cast<float>(height)}];
    }

    void window::set_position(int x, int y)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, x, y] { return set_position(x, y); });
        }

        [m_impl->window setFrameOrigin:{static_cast<double>(x), static_cast<double>(y)}];
    }

    void window::clear(window_event event)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event] { return clear(event); });
        }

        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        const utils::autorelease_guard guard{};

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, event, id] { return remove(event, id); });
        }

        m_events.remove(event, id);
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS;
} // namespace saucer
