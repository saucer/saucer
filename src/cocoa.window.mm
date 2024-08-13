#include "cocoa.window.impl.hpp"

#include <mutex>

namespace saucer
{
    window::window(const options &) : m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;

        std::call_once(flag,
                       []()
                       {
                           impl::application = [NSApplication sharedApplication];
                           [impl::application setDelegate:[[AppDelegate alloc] init]];
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

        [m_impl->window center];
    }

    window::~window() = default;

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

        return [m_impl->window isKeyWindow];
    }

    bool window::minimized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return minimized(); }).get();
        }

        return [m_impl->window isMiniaturized];
    }

    bool window::maximized() const // NOLINT
    {
        // TODO
        return {};
    }

    bool window::resizable() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return resizable(); }).get();
        }

        return [m_impl->window styleMask] & NSWindowStyleMaskResizable;
    }

    bool window::decorations() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return decorations(); }).get();
        }

        return !([m_impl->window styleMask] & NSWindowStyleMaskBorderless);
    }

    bool window::always_on_top() const // NOLINT
    {
        // TODO
        return {};
    }

    std::string window::title() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return title(); }).get();
        }

        return [[m_impl->window title] UTF8String];
    }

    std::pair<int, int> window::size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return size(); }).get();
        }

        auto [width, height] = [m_impl->window frame].size;
        return {width, height};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return max_size(); }).get();
        }

        auto [width, height] = [m_impl->window maxSize];
        return {width, height};
    }

    std::pair<int, int> window::min_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this] { return min_size(); }).get();
        }

        auto [width, height] = [m_impl->window minSize];
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

    void window::start_resize(window_edge edge) // NOLINT
    {
        (void)edge;
        // TODO
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

        // TODO: Not sure if this is right yet, when maximized it takes full size (minus dock size), but other MacOS
        // applications usually go to their own virtual desktop when maximized.

        [m_impl->window setIsZoomed:static_cast<BOOL>(enabled)];
    }

    void window::set_resizable(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_resizable(enabled); }).get();
        }

        static constexpr auto flag = NSWindowStyleMaskResizable;
        auto mask                  = [m_impl->window styleMask];

        if (!enabled)
        {
            mask &= ~flag;
        }
        else
        {
            mask |= flag;
        }

        // TODO: I can't get the resize cursor to show up at all...
        [m_impl->window setStyleMask:mask];
    }

    void window::set_decorations(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled] { return set_decorations(enabled); }).get();
        }

        static constexpr auto flag = NSWindowStyleMaskBorderless | NSWindowStyleMaskTitled;
        auto mask                  = [m_impl->window styleMask];

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

    void window::set_always_on_top(bool enabled) // NOLINT
    {
        // TODO
    }

    void window::set_icon(const icon &) // NOLINT
    {
        // TODO
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

        auto frame = [m_impl->window frame];
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

    template <>
    void window::run<true>()
    {
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
} // namespace saucer
