#include "cocoa.window.impl.hpp"

#include "instantiate.hpp"

#include "cocoa.app.impl.hpp"
#include "cocoa.icon.impl.hpp"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

namespace saucer
{
    using impl = window::impl;

    impl::impl() = default;

    result<> impl::init_platform()
    {
        const utils::autorelease_guard guard{};

        platform = std::make_unique<native>();

        platform->window   = [[SaucerWindow alloc] initWithParent:this
                                                    contentRect:NSMakeRect(0, 0, 800, 600)
                                                      styleMask:mask
                                                        backing:NSBackingStoreBuffered
                                                          defer:NO];
        platform->delegate = [[WindowDelegate alloc] initWithParent:this];

        platform->window.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;
        [platform->window.contentView setAutoresizesSubviews:YES];

        [platform->window setDelegate:platform->delegate.get()];
        [platform->window center];

        parent->invoke([this] { [platform->delegate.get() windowDidResize:reinterpret_cast<NSNotification *>(0xDEADBEEF)]; });
        set_resizable(true);

        return {};
    }

    impl::~impl()
    {
        const utils::autorelease_guard guard{};

        if (!platform)
        {
            return;
        }

        // We hide-on-close, so we call trigger two different close calls to properly quit.
        close();

        [platform->window close];
    }

    template <window::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    bool impl::visible() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.isVisible;
    }

    bool impl::focused() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.isKeyWindow;
    }

    bool impl::minimized() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.isMiniaturized;
    }

    bool impl::maximized() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.isZoomed;
    }

    bool impl::resizable() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.styleMask & NSWindowStyleMaskResizable;
    }

    bool impl::fullscreen() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.styleMask & NSWindowStyleMaskFullScreen;
    }

    bool impl::always_on_top() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.level == kCGMaximumWindowLevelKey;
    }

    bool impl::click_through() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.ignoresMouseEvents;
    }

    std::string impl::title() const
    {
        const utils::autorelease_guard guard{};
        return platform->window.title.UTF8String;
    }

    color impl::background() const
    {
        const auto guard  = utils::autorelease_guard{};
        auto *const color = [platform->window.backgroundColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];

        return {
            .r = static_cast<std::uint8_t>(color.redComponent * 255.f),
            .g = static_cast<std::uint8_t>(color.greenComponent * 255.f),
            .b = static_cast<std::uint8_t>(color.blueComponent * 255.f),
            .a = static_cast<std::uint8_t>(color.alphaComponent * 255.f),
        };
    }

    window::decoration impl::decorations() const
    {
        using enum decoration;
        const utils::autorelease_guard guard{};

        if (platform->window.styleMask == NSWindowStyleMaskBorderless)
        {
            return none;
        }

        if (platform->window.styleMask & NSWindowStyleMaskFullSizeContentView)
        {
            return partial;
        }

        return full;
    }

    size impl::size() const
    {
        const auto guard           = utils::autorelease_guard{};
        const auto [width, height] = platform->window.contentView.frame.size;

        return {.w = static_cast<int>(width), .h = static_cast<int>(height)};
    }

    size impl::max_size() const
    {
        const auto guard           = utils::autorelease_guard{};
        const auto [width, height] = platform->window.contentMaxSize;

        return {.w = static_cast<int>(width), .h = static_cast<int>(height)};
    }

    size impl::min_size() const
    {
        const auto guard           = utils::autorelease_guard{};
        const auto [width, height] = platform->window.contentMinSize;

        return {.w = static_cast<int>(width), .h = static_cast<int>(height)};
    }

    position impl::position() const
    {
        const auto guard  = utils::autorelease_guard{};
        const auto [x, y] = platform->window.frame.origin;

        return {.x = static_cast<int>(x), .y = static_cast<int>(y)};
    }

    std::optional<screen> impl::screen() const
    {
        const auto guard   = utils::autorelease_guard{};
        auto *const screen = platform->window.screen;

        if (!screen)
        {
            return std::nullopt;
        }

        return application::impl::native::convert(screen);
    }

    void impl::hide() const
    {
        const utils::autorelease_guard guard{};
        [platform->window orderOut:nil];
    }

    void impl::show() const
    {
        const utils::autorelease_guard guard{};

        parent->native<false>()->platform->instances[platform->window] = true;
        [platform->window makeKeyAndOrderFront:nil];
    }

    void impl::close() const
    {
        const utils::autorelease_guard guard{};
        [platform->delegate.get() windowShouldClose:platform->window];
    }

    void impl::focus() const
    {
        const utils::autorelease_guard guard{};
        [platform->window makeKeyWindow];
    }

    void impl::start_drag() const
    {
        const utils::autorelease_guard guard{};
        [platform->window performWindowDragWithEvent:NSApp.currentEvent];
    }

    void impl::start_resize(edge edge) // NOLINT(*-function-const)
    {
        platform->edge.emplace(edge);
    }

    void impl::set_minimized(bool enabled) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->window setIsMiniaturized:static_cast<BOOL>(enabled)];
    }

    void impl::set_maximized(bool enabled) // NOLINT(*-function-const)
    {
        [platform->window setIsZoomed:static_cast<BOOL>(enabled)];
    }

    void impl::set_resizable(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard           = utils::autorelease_guard{};
        static constexpr auto flag = NSWindowStyleMaskResizable;

        if (!enabled)
        {
            platform->masks &= ~flag;
        }
        else
        {
            platform->masks |= flag;
        }

        [platform->window setStyleMask:mask | platform->masks];
    }

    void impl::set_fullscreen(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard = utils::autorelease_guard{};

        if (fullscreen() == enabled)
        {
            return;
        }

        [platform->window toggleFullScreen:nil];
    }

    void impl::set_always_on_top(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard       = utils::autorelease_guard{};
        platform->window.level = enabled ? kCGMaximumWindowLevelKey : kCGNormalWindowLevelKey;
    }

    void impl::set_click_through(bool enabled) // NOLINT(*-function-const)
    {
        const auto guard                    = utils::autorelease_guard{};
        platform->window.ignoresMouseEvents = static_cast<BOOL>(enabled);
    }

    void impl::set_icon(const icon &icon) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

        if (icon.empty())
        {
            return;
        }

        auto *const view = [NSImageView imageViewWithImage:icon.native<false>()->icon.get()];
        auto *const tile = parent->native<false>()->platform->application.dockTile;

        [tile setContentView:view];
        [tile display];
    }

    void impl::set_title(cstring_view title) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto [r, g, b, a] = color;

        [platform->window setBackgroundColor:[NSColor colorWithSRGBRed:static_cast<CGFloat>(r) / 255.f
                                                                 green:static_cast<CGFloat>(g) / 255.f
                                                                  blue:static_cast<CGFloat>(b) / 255.f
                                                                 alpha:static_cast<CGFloat>(a) / 255.f]];
    }

    void impl::set_decorations(decoration decoration) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};

        if (decoration == decoration::none)
        {
            [platform->window setStyleMask:NSWindowStyleMaskBorderless];
            return;
        }

        static constexpr auto flag = NSWindowStyleMaskFullSizeContentView;
        const auto hidden          = static_cast<BOOL>(decoration == decoration::partial);

        if (hidden)
        {
            platform->masks |= flag;
        }
        else
        {
            platform->masks &= ~flag;
        }

        [platform->window setStyleMask:mask | platform->masks];

        [platform->window standardWindowButton:NSWindowZoomButton].hidden        = hidden;
        [platform->window standardWindowButton:NSWindowCloseButton].hidden       = hidden;
        [platform->window standardWindowButton:NSWindowMiniaturizeButton].hidden = hidden;

        platform->window.titlebarAppearsTransparent = hidden;
        platform->window.titleVisibility            = hidden ? NSWindowTitleHidden : NSWindowTitleVisible;
    }

    void impl::set_size(saucer::size size) // NOLINT(*-function-const)
    {
        const auto guard = utils::autorelease_guard{};
        [platform->window setContentSize:{.width = static_cast<float>(size.w), .height = static_cast<float>(size.h)}];
    }

    void impl::set_max_size(saucer::size size) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->window setContentMaxSize:{static_cast<float>(size.w), static_cast<float>(size.h)}];
    }

    void impl::set_min_size(saucer::size size) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->window setContentMinSize:{static_cast<float>(size.w), static_cast<float>(size.h)}];
    }

    void impl::set_position(saucer::position position) // NOLINT(*-function-const)
    {
        const utils::autorelease_guard guard{};
        [platform->window setFrameOrigin:{static_cast<double>(position.x), static_cast<double>(position.y)}];
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT);
} // namespace saucer
