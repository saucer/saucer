#include "win32.window.impl.hpp"

#include "win32.utils.hpp"
#include "win32.icon.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <fmt/core.h>
#include <flagpp/flags.hpp>

#include <winuser.h>
#include <versionhelpers.h>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    window::window(const preferences &) : m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;

        std::call_once(flag,
                       []()
                       {
                           impl::instance = GetModuleHandleW(nullptr);

                           //? Register the window class, later referred to by passing `lpClassName` = "Saucer"
                           WNDCLASSW wnd_class{};

                           wnd_class.hInstance     = impl::instance;
                           wnd_class.lpszClassName = L"Saucer";
                           wnd_class.lpfnWndProc   = impl::wnd_proc;

                           if (RegisterClassW(&wnd_class))
                           {
                               return;
                           }

                           assert(false && "RegisterClassW() failed");
                       });

        assert(impl::instance && "Construction outside of the main-thread is not permitted");

        const auto dw_style = IsWindows8OrGreater() ? WS_EX_NOREDIRECTIONBITMAP : 0;

        m_impl->hwnd = CreateWindowExW(dw_style,            //
                                       L"Saucer",           //
                                       L"Saucer Window",    //
                                       WS_OVERLAPPEDWINDOW, //
                                       CW_USEDEFAULT,       //
                                       CW_USEDEFAULT,       //
                                       CW_USEDEFAULT,       //
                                       CW_USEDEFAULT,       //
                                       nullptr,             //
                                       nullptr,             //
                                       impl::instance,      //
                                       nullptr);

        assert(m_impl->hwnd && "CreateWindowExW() failed");

        utils::set_dpi_awareness();
        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        impl::instances++;
    }

    window::~window()
    {
        DestroyWindow(m_impl->hwnd);
    }

    void window::dispatch(callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostMessage(m_impl->hwnd, impl::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
    }

    bool window::focused() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return focused(); }).get();
        }

        return m_impl->hwnd == GetForegroundWindow();
    }

    bool window::minimized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return minimized(); }).get();
        }

        return IsIconic(m_impl->hwnd);
    }

    bool window::maximized() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return maximized(); }).get();
        }

        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);

        GetWindowPlacement(m_impl->hwnd, &placement);

        return placement.showCmd == SW_SHOWMAXIMIZED;
    }

    bool window::resizable() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return resizable(); }).get();
        }

        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & WS_THICKFRAME;
    }

    bool window::decorations() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return decorations(); }).get();
        }

        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & WS_CAPTION;
    }

    bool window::always_on_top() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return always_on_top(); }).get();
        }

        return GetWindowLong(m_impl->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
    }

    std::string window::title() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return title(); }).get();
        }

        std::wstring title;

        title.resize(GetWindowTextLengthW(m_impl->hwnd));
        GetWindowTextW(m_impl->hwnd, title.data(), static_cast<int>(title.capacity()));

        return utils::narrow(title);
    }

    std::pair<int, int> window::size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return size(); }).get();
        }

        RECT rect;
        GetWindowRect(m_impl->hwnd, &rect);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return max_size(); }).get();
        }

        const auto width  = GetSystemMetrics(SM_CXMAXTRACK);
        const auto height = GetSystemMetrics(SM_CYMAXTRACK);

        return m_impl->max_size.value_or(std::make_pair(width, height));
    }

    std::pair<int, int> window::min_size() const
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { return min_size(); }).get();
        }

        const auto width  = GetSystemMetrics(SM_CXMINTRACK);
        const auto height = GetSystemMetrics(SM_CYMINTRACK);

        return m_impl->min_size.value_or(std::make_pair(width, height));
    }

    void window::hide()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { hide(); }).get();
        }

        ShowWindow(m_impl->hwnd, SW_HIDE);
    }

    void window::show()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { show(); }).get();
        }

        ShowWindow(m_impl->hwnd, SW_SHOW);
    }

    void window::close()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { close(); }).get();
        }

        PostMessage(m_impl->hwnd, WM_CLOSE, 0, 0);
    }

    void window::focus()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { focus(); }).get();
        }

        SetForegroundWindow(m_impl->hwnd);
    }

    // Kudos to Qt for serving as a really good reference here:
    // https://github.com/qt/qtbase/blob/37b6f941ee210e0bc4d65e8e700b6e19eb89c414/src/plugins/platforms/windows/qwindowswindow.cpp#L3028

    void window::start_drag()
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this]() { start_drag(); }).get();
        }

        ReleaseCapture();
        PostMessage(m_impl->hwnd, WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    }

    void window::start_resize(saucer::window_edge edge)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, edge]() { start_resize(edge); }).get();
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
        PostMessage(m_impl->hwnd, WM_SYSCOMMAND, translated, 0);
    }

    void window::set_minimized(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled]() { set_minimized(enabled); }).get();
        }

        ShowWindow(m_impl->hwnd, enabled ? SW_MINIMIZE : SW_RESTORE);
    }

    void window::set_maximized(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled]() { set_maximized(enabled); }).get();
        }

        ShowWindow(m_impl->hwnd, enabled ? SW_MAXIMIZE : SW_RESTORE);
    }

    void window::set_resizable(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled]() { set_resizable(enabled); }).get();
        }

        static constexpr auto flags = WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        auto current_style          = GetWindowLongW(m_impl->hwnd, GWL_STYLE);

        if (enabled)
        {
            current_style |= flags;
        }
        else
        {
            current_style &= ~flags;
        }

        SetWindowLongW(m_impl->hwnd, GWL_STYLE, current_style);
    }

    void window::set_decorations(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled]() { set_decorations(enabled); }).get();
        }

        static constexpr auto flag = WS_TILEDWINDOW;
        auto current_style         = GetWindowLongW(m_impl->hwnd, GWL_STYLE);

        if (enabled)
        {
            current_style |= flag;
        }
        else
        {
            current_style &= ~flag;
        }

        SetWindowLongW(m_impl->hwnd, GWL_STYLE, current_style);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, enabled]() { set_always_on_top(enabled); }).get();
        }

        SetWindowPos(m_impl->hwnd, enabled ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!impl::is_thread_safe())
        {
            return dispatch([this, icon]() { return set_icon(icon); }).get();
        }

        HICON handle{};

        if (icon.m_impl->bitmap->GetHICON(&handle) != Gdiplus::Status::Ok)
        {
            return;
        }

        SendMessage(m_impl->hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(handle));

        DestroyIcon(handle);
    }

    void window::set_title(const std::string &title)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, title]() { return set_title(title); }).get();
        }

        SetWindowTextW(m_impl->hwnd, utils::widen(title).c_str());
    }

    void window::set_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height]() { set_size(width, height); }).get();
        }

        SetWindowPos(m_impl->hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    void window::set_max_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height]() { set_max_size(width, height); }).get();
        }

        m_impl->max_size = {width, height};
    }

    void window::set_min_size(int width, int height)
    {
        if (!impl::is_thread_safe())
        {
            return dispatch([this, width, height]() { set_min_size(width, height); }).get();
        }

        m_impl->min_size = {width, height};
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

    template <>
    void window::run<true>()
    {
        MSG msg;

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    template <>
    void window::run<false>()
    {
        MSG msg;

        if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            return;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    INSTANTIATE_EVENTS(window, 7, window_event)
} // namespace saucer
