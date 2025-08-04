#include "win32.window.impl.hpp"

#include "win32.utils.hpp"
#include "win32.app.impl.hpp"
#include "win32.icon.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <rebind/enum.hpp>

#include <dwmapi.h>

namespace saucer
{
    static constexpr auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    using impl = window::impl;

    impl::impl() = default;

    bool impl::init_platform()
    {
        utils::set_dpi_awareness();

        auto *const hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,                     //
                                           parent->native<false>()->platform->id.c_str(), //
                                           L"",                                           //
                                           style,                                         //
                                           CW_USEDEFAULT,                                 //
                                           CW_USEDEFAULT,                                 //
                                           CW_USEDEFAULT,                                 //
                                           CW_USEDEFAULT,                                 //
                                           nullptr,                                       //
                                           nullptr,                                       //
                                           parent->native<false>()->platform->handle,     //
                                           nullptr);

        if (!hwnd)
        {
            return false;
        }

        platform = std::make_unique<native>();

        platform->hwnd = hwnd;
        platform->hook = {platform->hwnd.get(), native::wnd_proc};

        SetWindowLongPtrW(platform->hwnd.get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        set_resizable(true);

        return true;
    }

    impl::~impl()
    {
        close();
        SetWindowLongPtrW(platform->hwnd.get(), GWLP_USERDATA, 0);
    }

    template <window::event Event>
    void impl::setup()
    {
    }

    bool impl::visible() const
    {
        return IsWindowVisible(platform->hwnd.get());
    }

    bool impl::focused() const
    {
        return platform->hwnd.get() == GetForegroundWindow();
    }

    bool impl::minimized() const
    {
        return IsIconic(platform->hwnd.get());
    }

    bool impl::maximized() const
    {
        return IsZoomed(platform->hwnd.get());
    }

    bool impl::resizable() const
    {
        return GetWindowLongPtrW(platform->hwnd.get(), GWL_STYLE) & WS_THICKFRAME;
    }

    bool impl::always_on_top() const
    {
        return GetWindowLongPtrW(platform->hwnd.get(), GWL_EXSTYLE) & WS_EX_TOPMOST;
    }

    bool impl::click_through() const
    {
        return GetWindowLongPtrW(platform->hwnd.get(), GWL_EXSTYLE) & WS_EX_TRANSPARENT;
    }

    std::string impl::title() const
    {
        std::wstring title;

        title.resize(GetWindowTextLengthW(platform->hwnd.get()));
        GetWindowTextW(platform->hwnd.get(), title.data(), static_cast<int>(title.capacity()));

        return utils::narrow(title);
    }

    color impl::background() const // NOLINT(*-static)
    {
        // TODO: Implement
        return {};
    }

    window::decoration impl::decorations() const
    {
        using enum decoration;

        if (!(platform->styles & WS_CAPTION))
        {
            return none;
        }

        if (!platform->titlebar)
        {
            return partial;
        }

        return full;
    }

    size impl::size() const
    {
        RECT rect;
        GetWindowRect(platform->hwnd.get(), &rect);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    size impl::max_size() const
    {
        const auto width  = GetSystemMetrics(SM_CXMAXTRACK);
        const auto height = GetSystemMetrics(SM_CYMAXTRACK);

        return platform->max_size.value_or({.x = width, .y = height});
    }

    size impl::min_size() const
    {
        const auto width  = GetSystemMetrics(SM_CXMINTRACK);
        const auto height = GetSystemMetrics(SM_CYMINTRACK);

        return platform->min_size.value_or({.x = width, .y = height});
    }

    position impl::position() const
    {
        RECT rect{};
        GetWindowRect(platform->hwnd.get(), &rect);

        return {.x = rect.left, .y = rect.top};
    }

    std::optional<saucer::screen> impl::screen() const
    {
        auto *const monitor = MonitorFromWindow(platform->hwnd.get(), MONITOR_DEFAULTTONEAREST);

        if (!monitor)
        {
            return std::nullopt;
        }

        MONITORINFOEXW info{};
        info.cbSize = sizeof(MONITORINFOEXW);

        if (!GetMonitorInfo(monitor, &info))
        {
            return std::nullopt;
        }

        return application::impl::native::convert(info);
    }

    void impl::hide() const
    {
        ShowWindow(platform->hwnd.get(), SW_HIDE);
    }

    void impl::show() const
    {
        parent->native<false>()->platform->instances[platform->hwnd.get()] = true;
        ShowWindow(platform->hwnd.get(), SW_SHOW);
    }

    void impl::close() const
    {
        SendMessage(platform->hwnd.get(), WM_CLOSE, 0, 0);
    }

    void impl::focus() const
    {
        SetForegroundWindow(platform->hwnd.get());
    }

    // Kudos to Qt for serving as a really good reference here:
    // https://github.com/qt/qtbase/blob/37b6f941ee210e0bc4d65e8e700b6e19eb89c414/src/plugins/platforms/windows/qwindowswindow.cpp#L3028

    void impl::start_drag() const
    {
        ReleaseCapture();
        SendMessage(platform->hwnd.get(), WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    }

    void impl::start_resize(edge edge) // NOLINT(*-function-const)
    {
        DWORD translated{};

        switch (edge)
        {
            using enum window::edge;

        case top:
            translated = 0xF003; // SC_SIZETOP
            break;
        case bottom:
            translated = 0xF006; // SC_SIZEBOTTOM
            break;
        case left:
            translated = 0xF001; // SC_SIZELEFT;
            break;
        case right:
            translated = 0xF002; // SC_SIZERIGHT
            break;
        case top_left:
            translated = 0xF004; // SC_SIZETOPLEFT
            break;
        case top_right:
            translated = 0xF005; // SC_SIZETOPRIGHT
            break;
        case bottom_left:
            translated = 0xF007; // SC_SIZEBOTTOMLEFT
            break;
        case bottom_right:
            translated = 0xF008; // SC_SIZEBOTTOMRIGHT
            break;
        }

        ReleaseCapture();
        SendMessage(platform->hwnd.get(), WM_SYSCOMMAND, translated, 0);
    }

    void impl::set_minimized(bool enabled) // NOLINT(*-function-const)
    {
        ShowWindow(platform->hwnd.get(), enabled ? SW_MINIMIZE : SW_RESTORE);
    }

    void impl::set_maximized(bool enabled) // NOLINT(*-function-const)
    {
        ShowWindow(platform->hwnd.get(), enabled ? SW_MAXIMIZE : SW_RESTORE);
    }

    void impl::set_resizable(bool enabled) // NOLINT(*-function-const)
    {
        static constexpr auto flags = WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        if (enabled)
        {
            platform->styles |= flags;
        }
        else
        {
            platform->styles &= ~flags;
        }

        native::set_style(platform->hwnd.get(), style | platform->styles);
    }

    void impl::set_always_on_top(bool enabled) // NOLINT(*-function-const)
    {
        auto *parent = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
        SetWindowPos(platform->hwnd.get(), parent, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
    }

    void impl::set_click_through(bool enabled) // NOLINT(*-function-const)
    {
        static constexpr auto flags = WS_EX_TRANSPARENT | WS_EX_LAYERED;
        auto current                = GetWindowLongPtr(platform->hwnd.get(), GWL_EXSTYLE);

        if (enabled)
        {
            current |= flags;
        }
        else
        {
            current &= ~flags;
        }

        SetWindowLongPtrW(platform->hwnd.get(), GWL_EXSTYLE, current);

        if (!enabled)
        {
            return;
        }

        SetLayeredWindowAttributes(platform->hwnd.get(), RGB(255, 255, 255), 255, 0);
    }

    void impl::set_icon(const icon &icon) // NOLINT(*-function-const)
    {
        if (icon.empty())
        {
            return;
        }

        if (icon.native<false>()->bitmap->GetHICON(&platform->icon.reset()) != Gdiplus::Status::Ok)
        {
            return;
        }

        SendMessage(platform->hwnd.get(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(platform->icon.get()));
    }

    void impl::set_title(const std::string &title) // NOLINT(*-function-const)
    {
        SetWindowTextW(platform->hwnd.get(), utils::widen(title).c_str());
    }

    void impl::set_background(color)
    {
        // TODO: Implement
    }

    void impl::set_decorations(decoration decoration) // NOLINT(*-function-const)
    {
        const auto decorated = decoration != decoration::none;
        const auto titlebar  = decoration != decoration::partial;

        platform->titlebar = titlebar;

        if (!decorated)
        {
            native::set_style(platform->hwnd.get(), 0);
            return;
        }

        native::set_style(platform->hwnd.get(), style | platform->styles);
        utils::extend_frame(platform->hwnd.get(), {0, 0, titlebar ? 0 : 2, 0});

        SetWindowPos(platform->hwnd.get(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void impl::set_size(saucer::size size) // NOLINT(*-function-const)
    {
        SetWindowPos(platform->hwnd.get(), nullptr, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void impl::set_max_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->max_size = size;
    }

    void impl::set_min_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->min_size = size;
    }

    void impl::set_position(saucer::position position) // NOLINT(*-function-const)
    {
        SetWindowPos(platform->hwnd.get(), nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT);
} // namespace saucer
