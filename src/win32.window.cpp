#include "win32.window.impl.hpp"

#include "win32.error.hpp"
#include "win32.utils.hpp"

#include "win32.app.impl.hpp"
#include "win32.icon.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <rebind/enum.hpp>

#include <dwmapi.h>
#include <winrt/windows.ui.viewmanagement.core.h>

namespace saucer
{
    using impl = window::impl;

    impl::impl() = default;

    result<> impl::init_platform()
    {
        using namespace winrt::Windows::UI::ViewManagement;

        utils::set_dpi_awareness();

        if (static auto once{true}; once)
        {
            native::windows_build = utils::version().dwBuildNumber;
            once                  = false;
        }

        utils::window_handle hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,                     //
                                                    parent->native<false>()->platform->id.c_str(), //
                                                    L"",                                           //
                                                    window_flags::standard,                        //
                                                    CW_USEDEFAULT,                                 //
                                                    CW_USEDEFAULT,                                 //
                                                    CW_USEDEFAULT,                                 //
                                                    CW_USEDEFAULT,                                 //
                                                    nullptr,                                       //
                                                    nullptr,                                       //
                                                    parent->native<false>()->platform->handle,     //
                                                    nullptr);

        if (!hwnd.get())
        {
            return err(GetLastError());
        }

        auto compositor    = utils::compositor{};
        auto window_target = utils::create_window_target(compositor, hwnd.get());

        if (!window_target.has_value())
        {
            return err(window_target);
        }

        platform = std::make_unique<native>();

        platform->hwnd          = std::move(hwnd);
        platform->window_target = std::move(window_target.value());
        platform->background    = compositor.CreateColorBrush(UISettings{}.GetColorValue(UIColorType::Background));
        platform->hook          = {platform->hwnd.get(), native::wnd_proc};

        const auto atom = application::impl::native::ATOM_WINDOW.get();
        SetPropW(platform->hwnd.get(), MAKEINTATOM(atom), reinterpret_cast<HANDLE>(this));

        auto root = compositor.CreateSpriteVisual();
        {
            root.Brush(platform->background);
        }
        platform->window_target.Root(root);

        set_resizable(true);

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        close();

        const auto atom = application::impl::native::ATOM_WINDOW.get();
        RemovePropW(platform->hwnd.get(), MAKEINTATOM(atom));
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

    bool impl::fullscreen() const
    {
        const auto monitor = screen();

        if (!monitor.has_value())
        {
            assert(false);
            return false;
        }

        RECT rect{};
        GetWindowRect(platform->hwnd.get(), &rect);

        const auto pos  = monitor->position;
        const auto size = monitor->size;

        return pos.x == rect.left && pos.y == rect.top && size.w == rect.right - rect.left && size.h == rect.bottom - rect.top;
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
        auto [a, r, g, b] = platform->background.Color();
        return {.r = r, .g = g, .b = b, .a = a};
    }

    window::decoration impl::decorations() const
    {
        return platform->flags.decorations;
    }

    size impl::size() const
    {
        RECT rect;
        GetClientRect(platform->hwnd.get(), &rect);
        return {.w = rect.right - rect.left, .h = rect.bottom - rect.top};
    }

    size impl::max_size() const
    {
        const auto width  = GetSystemMetrics(SM_CXMAXTRACK);
        const auto height = GetSystemMetrics(SM_CYMAXTRACK);

        return platform->offset<offset::sub>(platform->max_size.value_or({.w = width, .h = height}));
    }

    size impl::min_size() const
    {
        const auto width  = GetSystemMetrics(SM_CXMINTRACK);
        const auto height = GetSystemMetrics(SM_CYMINTRACK);

        return platform->offset<offset::sub>(platform->min_size.value_or({.w = width, .h = height}));
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
        platform->flags.resizable = enabled;
        platform->flags.apply(platform->hwnd.get());
    }

    void impl::set_fullscreen(bool enabled) // NOLINT(*-function-const)
    {
        auto *const hwnd = platform->hwnd.get();

        platform->flags.fullscreen = enabled;
        platform->flags.apply(hwnd);

        if (!enabled && platform->prev_placement.has_value())
        {
            SetWindowPlacement(hwnd, &platform->prev_placement.value());
        }

        if (!enabled)
        {
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            return;
        }

        platform->prev_placement.emplace(WINDOWPLACEMENT{.length = sizeof(WINDOWPLACEMENT)});
        GetWindowPlacement(hwnd, &platform->prev_placement.value());

        const auto monitor = screen();

        if (!monitor.has_value())
        {
            return;
        }

        const auto pos  = monitor->position;
        const auto size = monitor->size;

        SetWindowPos(hwnd, HWND_TOP, pos.x, pos.y, size.w, size.h, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }

    void impl::set_always_on_top(bool enabled) // NOLINT(*-function-const)
    {
        auto *parent = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
        SetWindowPos(platform->hwnd.get(), parent, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
    }

    void impl::set_click_through(bool enabled) // NOLINT(*-function-const)
    {
        platform->flags.click_through = enabled;
        platform->flags.apply(platform->hwnd.get());

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

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        auto [r, g, b, a] = color;
        platform->background.Color({.A = a, .R = r, .G = g, .B = b});
    }

    void impl::set_decorations(decoration decoration) // NOLINT(*-function-const)
    {
        platform->flags.decorations = decoration;
        platform->flags.apply(platform->hwnd.get());

        if (decoration == decoration::none)
        {
            return;
        }

        utils::extend_frame(platform->hwnd.get(), {0, 0, decoration == decoration::partial ? 2 : 0, 0});
        SetWindowPos(platform->hwnd.get(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void impl::set_size(saucer::size size) // NOLINT(*-function-const)
    {
        auto [width, height] = platform->offset<offset::add>(size);
        SetWindowPos(platform->hwnd.get(), nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void impl::set_max_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->max_size = platform->offset<offset::add>(size);
    }

    void impl::set_min_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->min_size = platform->offset<offset::add>(size);
    }

    void impl::set_position(saucer::position position) // NOLINT(*-function-const)
    {
        SetWindowPos(platform->hwnd.get(), nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT);
} // namespace saucer
