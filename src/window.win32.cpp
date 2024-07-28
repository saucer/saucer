#include "window.win32.impl.hpp"

#include "utils.win32.hpp"
#include "icon.win32.impl.hpp"

#include "instantiate.hpp"

#include <fmt/core.h>
#include <flagpp/flags.hpp>

#include <winuser.h>
#include <versionhelpers.h>

template <>
constexpr bool flagpp::enabled<saucer::window_edge> = true;

namespace saucer
{
    window::window(const options &) : m_impl(std::make_unique<impl>())
    {
        static HMODULE instance;
        static WNDCLASSW wnd_class;

        m_impl->creation_thread = std::this_thread::get_id();

        if (!instance)
        {
            instance = GetModuleHandleW(nullptr);

            //? Register the window class, later referred to by passing `lpClassName` = "Saucer"

            wnd_class.hInstance     = instance;
            wnd_class.lpszClassName = L"Saucer";
            wnd_class.lpfnWndProc   = m_impl->wnd_proc;

            if (!RegisterClassW(&wnd_class))
            {
                utils::throw_error("RegisterClassW() failed");
            }
        }

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
                                       instance,            //
                                       nullptr);

        if (!m_impl->hwnd)
        {
            utils::throw_error("CreateWindowExW() failed");
        }

        utils::set_dpi_awareness();

        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        impl::instances++;
    }

    window::~window()
    {
        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
        DestroyWindow(m_impl->hwnd);
    }

    template <>
    void window::run<false>();

    template <>
    void window::run<true>();

    bool window::focused() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return focused(); });
        }

        return m_impl->hwnd == GetForegroundWindow();
    }

    bool window::minimized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return minimized(); });
        }

        return IsIconic(m_impl->hwnd);
    }

    bool window::maximized() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return maximized(); });
        }

        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);

        GetWindowPlacement(m_impl->hwnd, &placement);

        return placement.showCmd == SW_SHOWMAXIMIZED;
    }

    bool window::resizable() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return resizable(); });
        }

        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    bool window::decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return decorations(); });
        }

        const auto flags = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & flags;
    }

    bool window::always_on_top() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return always_on_top(); });
        }

        return GetWindowLong(m_impl->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
    }

    color window::background() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return background(); });
        }

        std::promise<color> promise;
        auto result = promise.get_future();

        m_impl->post(new get_background_message{&promise}, impl::WM_GET_BACKGROUND);

        while (result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            run<false>();
        }

        return result.get();
    }

    std::string window::title() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return title(); });
        }

        std::wstring title;

        title.resize(GetWindowTextLengthW(m_impl->hwnd));
        GetWindowTextW(m_impl->hwnd, title.data(), static_cast<int>(title.capacity()));

        return utils::narrow(title);
    }

    std::pair<int, int> window::size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return size(); });
        }

        RECT rect;
        GetWindowRect(m_impl->hwnd, &rect);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return max_size(); });
        }

        const auto width  = GetSystemMetrics(SM_CXMAXTRACK);
        const auto height = GetSystemMetrics(SM_CYMAXTRACK);

        return m_impl->max_size.value_or(std::make_pair(width, height));
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return min_size(); });
        }

        const auto width  = GetSystemMetrics(SM_CXMINTRACK);
        const auto height = GetSystemMetrics(SM_CYMINTRACK);

        return m_impl->min_size.value_or(std::make_pair(width, height));
    }

    void window::hide()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { hide(); });
        }

        ShowWindow(m_impl->hwnd, SW_HIDE);
    }

    void window::show()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { show(); });
        }

        ShowWindow(m_impl->hwnd, SW_SHOW);
    }

    void window::close()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { close(); });
        }

        DestroyWindow(m_impl->hwnd);
    }

    void window::focus()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { focus(); });
        }

        SetForegroundWindow(m_impl->hwnd);
    }

    // Kudos to Qt for serving as a really good reference here:
    // https://github.com/qt/qtbase/blob/37b6f941ee210e0bc4d65e8e700b6e19eb89c414/src/plugins/platforms/windows/qwindowswindow.cpp#L3028

    void window::start_drag()
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { start_drag(); });
        }

        ReleaseCapture();
        PostMessage(m_impl->hwnd, WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    }

    void window::start_resize(saucer::window_edge edge)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, edge] { start_resize(edge); });
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
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_minimized(enabled); });
        }

        ShowWindow(m_impl->hwnd, enabled ? SW_MINIMIZE : SW_RESTORE);
    }

    void window::set_maximized(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_maximized(enabled); });
        }

        ShowWindow(m_impl->hwnd, enabled ? SW_MAXIMIZE : SW_RESTORE);
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_resizable(enabled); });
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
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_decorations(enabled); });
        }

        static constexpr auto flags = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
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

    void window::set_always_on_top(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_always_on_top(enabled); });
        }

        SetWindowPos(m_impl->hwnd, enabled ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    void window::set_icon(const icon &icon)
    {
        if (icon.empty())
        {
            return;
        }

        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, icon] { return set_icon(icon); });
        }

        HICON handle{};

        if (icon.m_impl->bitmap->GetHICON(&handle) != Gdiplus::Status::Ok)
        {
            return;
        }

        SendMessage(m_impl->hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(handle));
        DestroyIcon(handle);
    }

    void window::set_title(const std::string &title)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, title] { return set_title(title); });
        }

        SetWindowTextW(m_impl->hwnd, utils::widen(title).c_str());
    }

    void window::set_background(const color &background)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, background] { return set_background(background); });
        }

        std::promise<void> promise;
        auto result = promise.get_future();

        m_impl->post(new set_background_message{background, &promise}, impl::WM_SET_BACKGROUND);

        while (result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            run<false>();
        }

        return result.get();
    }

    void window::set_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, width, height] { set_size(width, height); });
        }

        SetWindowPos(m_impl->hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    void window::set_max_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, width, height] { set_max_size(width, height); });
        }

        m_impl->max_size = {width, height};
    }

    void window::set_min_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, width, height] { set_min_size(width, height); });
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
    void window::once(events::type_t<Event> callback)
    {
        m_events.at<Event>().once(std::move(callback));
    }

    template <window_event Event>
    std::uint64_t window::on(events::type_t<Event> callback)
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

    INSTANTIATE_EVENTS(window, 6, window_event)
} // namespace saucer
