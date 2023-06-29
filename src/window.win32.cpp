#include "window.hpp"
#include "utils.win32.hpp"
#include "window.win32.impl.hpp"

#include <winuser.h>
#include <fmt/core.h>
#include <VersionHelpers.h>

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

            // ? Register the window class, later referred to by passing `lpClassName` = "Saucer"

            wnd_class.hInstance = instance;
            wnd_class.lpszClassName = L"Saucer";
            wnd_class.lpfnWndProc = m_impl->wnd_proc;

            if (!RegisterClassW(&wnd_class))
            {
                throw std::runtime_error(fmt::format("RegisterClassW() failed: {}", last_error()));
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
            throw std::runtime_error(fmt::format("CreateWindowExW() failed: {}", last_error()));
        }

        set_dpi_awareness();

        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        impl::open_windows++;
    }

    window::~window()
    {
        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
        DestroyWindow(m_impl->hwnd);
    }

    bool window::resizable() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return resizable(); });
        }

        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    std::string window::title() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return title(); });
        }

        auto title_length = GetWindowTextLengthW(m_impl->hwnd) + 1;
        auto *title = new WCHAR[title_length];

        GetWindowTextW(m_impl->hwnd, title, title_length);
        auto rtn = narrow(title);

        delete[] title;
        return rtn;
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

    std::pair<int, int> window::size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return size(); });
        }

        RECT rect;
        GetClientRect(m_impl->hwnd, &rect);
        return std::pair<int, int>{rect.right - rect.left, rect.bottom - rect.top};
    }

    std::pair<int, int> window::max_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return max_size(); });
        }

        const auto max_track = GetSystemMetrics(SM_CXMAXTRACK);
        return m_impl->max_size.value_or(std::pair<int, int>{max_track, max_track});
    }

    std::pair<int, int> window::min_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return min_size(); });
        }

        const auto min_track = GetSystemMetrics(SM_CXMINTRACK);
        return m_impl->min_size.value_or(std::pair<int, int>{min_track, min_track});
    }

    color window::background() const
    {
        return m_impl->background_color;
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

        ShowWindow(m_impl->hwnd, SW_HIDE);
        PostQuitMessage(0);
    }

    void window::set_resizable(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_resizable(enabled); });
        }

        constexpr auto flags = WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        auto current_style = GetWindowLongW(m_impl->hwnd, GWL_STYLE);

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

    void window::set_title(const std::string &title)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, title] { return set_title(title); });
        }

        SetWindowTextW(m_impl->hwnd, widen(title).c_str());
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, enabled] { set_decorations(enabled); });
        }

        constexpr auto flags = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        auto current_style = GetWindowLongW(m_impl->hwnd, GWL_STYLE);

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

    void window::set_size(int width, int height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this, width, height] { set_size(width, height); });
        }

        SetWindowPos(m_impl->hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    void window::set_min_size(int width, int height)
    {
        m_impl->min_size = {width, height};
    }

    void window::set_max_size(int width, int height)
    {
        m_impl->max_size = {width, height};
    }

    void window::set_background(const color &color)
    {
        m_impl->set_background_color(color);
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::remove(window_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <>
    std::uint64_t window::on<window_event::close>(events::type_t<window_event::close> &&callback)
    {
        return m_events.at<window_event::close>().add(std::move(callback));
    }

    template <>
    std::uint64_t window::on<window_event::resize>(events::type_t<window_event::resize> &&callback)
    {
        return m_events.at<window_event::resize>().add(std::move(callback));
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

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
} // namespace saucer