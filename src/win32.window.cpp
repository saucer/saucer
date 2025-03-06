#include "win32.window.impl.hpp"

#include "win32.utils.hpp"
#include "win32.app.impl.hpp"
#include "win32.icon.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <rebind/enum.hpp>
#include <flagpp/flags.hpp>

#include <dwmapi.h>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    static constexpr auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS;

    window::window(const preferences &prefs) : m_impl(std::make_unique<impl>()), m_parent(prefs.application.value())
    {
        assert(m_parent->thread_safe() && "Construction outside of the main-thread is not permitted");

        m_impl->hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,             //
                                       m_parent->native<false>()->id.c_str(), //
                                       L"",                                   //
                                       style,                                 //
                                       CW_USEDEFAULT,                         //
                                       CW_USEDEFAULT,                         //
                                       CW_USEDEFAULT,                         //
                                       CW_USEDEFAULT,                         //
                                       nullptr,                               //
                                       nullptr,                               //
                                       m_parent->native<false>()->handle,     //
                                       nullptr);

        assert(m_impl->hwnd.get() && "CreateWindowExW() failed");

        set_resizable(true);

        utils::set_dpi_awareness();
        m_impl->o_wnd_proc = utils::overwrite_wndproc(m_impl->hwnd.get(), impl::wnd_proc);

        SetWindowLongPtrW(m_impl->hwnd.get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }

    window::~window()
    {
        for (const auto &event : rebind::enum_values<window_event>)
        {
            m_events.clear(event);
        }

        close();

        SetWindowLongPtrW(m_impl->hwnd.get(), GWLP_USERDATA, 0);
    }

    bool window::visible() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return visible(); });
        }

        return IsWindowVisible(m_impl->hwnd.get());
    }

    bool window::focused() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return focused(); });
        }

        return m_impl->hwnd.get() == GetForegroundWindow();
    }

    bool window::minimized() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return minimized(); });
        }

        return IsIconic(m_impl->hwnd.get());
    }

    bool window::maximized() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return maximized(); });
        }

        return IsZoomed(m_impl->hwnd.get());
    }

    bool window::resizable() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return resizable(); });
        }

        return GetWindowLongPtrW(m_impl->hwnd.get(), GWL_STYLE) & WS_THICKFRAME;
    }

    bool window::always_on_top() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return always_on_top(); });
        }

        return GetWindowLongPtrW(m_impl->hwnd.get(), GWL_EXSTYLE) & WS_EX_TOPMOST;
    }

    bool window::click_through() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return click_through(); });
        }

        return GetWindowLongPtrW(m_impl->hwnd.get(), GWL_EXSTYLE) & WS_EX_TRANSPARENT;
    }

    std::string window::title() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return title(); });
        }

        std::wstring title;

        title.resize(GetWindowTextLengthW(m_impl->hwnd.get()));
        GetWindowTextW(m_impl->hwnd.get(), title.data(), static_cast<int>(title.capacity()));

        return utils::narrow(title);
    }

    window_decoration window::decoration() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return decoration(); });
        }

        const auto style = GetWindowLongPtrW(m_impl->hwnd.get(), GWL_STYLE);

        if (!(style & WS_CAPTION))
        {
            return window_decoration::none;
        }

        if (!m_impl->titlebar)
        {
            return window_decoration::partial;
        }

        return window_decoration::full;
    }

    std::pair<int, int> window::size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return size(); });
        }

        RECT rect;
        GetWindowRect(m_impl->hwnd.get(), &rect);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return max_size(); });
        }

        const auto width  = GetSystemMetrics(SM_CXMAXTRACK);
        const auto height = GetSystemMetrics(SM_CYMAXTRACK);

        return m_impl->max_size.value_or(std::make_pair(width, height));
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return min_size(); });
        }

        const auto width  = GetSystemMetrics(SM_CXMINTRACK);
        const auto height = GetSystemMetrics(SM_CYMINTRACK);

        return m_impl->min_size.value_or(std::make_pair(width, height));
    }

    std::optional<screen> window::screen() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return screen(); });
        }

        auto *const monitor = MonitorFromWindow(m_impl->hwnd.get(), MONITOR_DEFAULTTONEAREST);

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

        return application::impl::convert(info);
    }

    std::pair<int, int> window::position() const
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return position(); });
        }

        RECT rect{};
        GetWindowRect(m_impl->hwnd.get(), &rect);

        return {rect.left, rect.top};
    }

    void window::hide()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { hide(); });
        }

        ShowWindow(m_impl->hwnd.get(), SW_HIDE);
    }

    void window::show()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { show(); });
        }

        m_parent->native<false>()->instances[m_impl->hwnd.get()] = true;
        ShowWindow(m_impl->hwnd.get(), SW_SHOW);
    }

    void window::close()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { close(); });
        }

        SendMessage(m_impl->hwnd.get(), WM_CLOSE, 0, 0);
    }

    void window::focus()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { focus(); });
        }

        SetForegroundWindow(m_impl->hwnd.get());
    }

    // Kudos to Qt for serving as a really good reference here:
    // https://github.com/qt/qtbase/blob/37b6f941ee210e0bc4d65e8e700b6e19eb89c414/src/plugins/platforms/windows/qwindowswindow.cpp#L3028

    void window::start_drag()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { start_drag(); });
        }

        ReleaseCapture();
        SendMessage(m_impl->hwnd.get(), WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    }

    void window::start_resize(saucer::window_edge edge)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, edge] { start_resize(edge); });
        }

        DWORD translated{};

        if (edge == window_edge::left)
        {
            translated = 0xF001; // SC_SIZELEFT;
        }
        else if (edge == window_edge::right)
        {
            translated = 0xF002; // SC_SIZERIGHT
        }
        else if (edge == window_edge::top)
        {
            translated = 0xF003; // SC_SIZETOP
        }
        else if (edge == (window_edge::top | window_edge::left))
        {
            translated = 0xF004; // SC_SIZETOPLEFT
        }
        else if (edge == (window_edge::top | window_edge::right))
        {
            translated = 0xF005; // SC_SIZETOPRIGHT
        }
        else if (edge == window_edge::bottom)
        {
            translated = 0xF006; // SC_SIZEBOTTOM
        }
        else if (edge == (window_edge::bottom | window_edge::left))
        {
            translated = 0xF007; // SC_SIZEBOTTOMLEFT
        }
        else if (edge == (window_edge::bottom | window_edge::right))
        {
            translated = 0xF008; // SC_SIZEBOTTOMRIGHT
        }

        ReleaseCapture();
        SendMessage(m_impl->hwnd.get(), WM_SYSCOMMAND, translated, 0);
    }

    void window::set_minimized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { set_minimized(enabled); });
        }

        ShowWindow(m_impl->hwnd.get(), enabled ? SW_MINIMIZE : SW_RESTORE);
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { set_maximized(enabled); });
        }

        ShowWindow(m_impl->hwnd.get(), enabled ? SW_MAXIMIZE : SW_RESTORE);
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { set_resizable(enabled); });
        }

        static constexpr auto flags = WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        if (enabled)
        {
            m_impl->styles |= flags;
        }
        else
        {
            m_impl->styles &= ~flags;
        }

        impl::set_style(m_impl->hwnd.get(), style | m_impl->styles);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { set_always_on_top(enabled); });
        }

        auto *parent = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
        SetWindowPos(m_impl->hwnd.get(), parent, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
    }

    void window::set_click_through(bool enabled)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, enabled] { set_click_through(enabled); });
        }

        static constexpr auto flags = WS_EX_TRANSPARENT | WS_EX_LAYERED;
        auto current                = GetWindowLongPtr(m_impl->hwnd.get(), GWL_EXSTYLE);

        if (enabled)
        {
            current |= flags;
        }
        else
        {
            current &= ~flags;
        }

        SetWindowLongPtrW(m_impl->hwnd.get(), GWL_EXSTYLE, current);

        if (!enabled)
        {
            return;
        }

        SetLayeredWindowAttributes(m_impl->hwnd.get(), RGB(255, 255, 255), 255, 0);
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, icon] { return set_icon(icon); });
        }

        if (icon.m_impl->bitmap->GetHICON(&m_impl->icon.reset()) != Gdiplus::Status::Ok)
        {
            return;
        }

        SendMessage(m_impl->hwnd.get(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_impl->icon.get()));
    }

    void window::set_title(const std::string &title)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, title] { return set_title(title); });
        }

        SetWindowTextW(m_impl->hwnd.get(), utils::widen(title).c_str());
    }

    void window::set_decoration(window_decoration decoration)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, decoration] { set_decoration(decoration); });
        }

        const auto decorated = decoration != window_decoration::none;
        const auto titlebar  = decoration != window_decoration::partial;

        m_impl->titlebar = titlebar;

        if (!decorated)
        {
            impl::set_style(m_impl->hwnd.get(), 0);
            return;
        }

        impl::set_style(m_impl->hwnd.get(), style | m_impl->styles);
        utils::extend_frame(m_impl->hwnd.get(), {0, 0, titlebar ? 0 : 2, 0});

        SetWindowPos(m_impl->hwnd.get(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void window::set_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { set_size(width, height); });
        }

        SetWindowPos(m_impl->hwnd.get(), nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void window::set_max_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { set_max_size(width, height); });
        }

        m_impl->max_size = {width, height};
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, width, height] { set_min_size(width, height); });
        }

        m_impl->min_size = {width, height};
    }

    void window::set_position(int x, int y)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, x, y] { set_position(x, y); });
        }

        SetWindowPos(m_impl->hwnd.get(), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <window_event Event>
    void window::once(events::type<Event> callback)
    {
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type<Event> callback)
    {
        return m_events.at<Event>().add(std::move(callback));
    }

    SAUCER_INSTANTIATE_EVENTS(7, window, window_event);
} // namespace saucer
