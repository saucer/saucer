#include "window.hpp"
#include "window.win32.impl.hpp"

namespace saucer
{
    window::window() : m_impl(std::make_unique<impl>())
    {
        if (!m_impl->instance)
        {
            m_impl->instance = GetModuleHandleW(nullptr);

            m_impl->wnd_class.lpszClassName = L"Saucer";
            m_impl->wnd_class.hInstance = m_impl->instance;
            m_impl->wnd_class.lpfnWndProc = m_impl->wnd_proc;

            if (!RegisterClassW(&m_impl->wnd_class))
            {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
            }
        }

        m_impl->hwnd = CreateWindowExW(0, L"Saucer", L"Saucer Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
                                       m_impl->instance, nullptr);

        if (!m_impl->hwnd)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
        }

        auto *shcoredll = LoadLibraryW(L"Shcore.dll");
        auto set_process_dpi_awareness = reinterpret_cast<HRESULT(CALLBACK *)(DWORD)>(GetProcAddress(shcoredll, "SetProcessDpiAwareness"));

        if (set_process_dpi_awareness)
        {
            set_process_dpi_awareness(2);
        }
        else
        {
            auto *user32dll = LoadLibraryW(L"user32.dll");
            auto set_process_dpi_aware = reinterpret_cast<bool(CALLBACK *)()>(GetProcAddress(user32dll, "SetProcessDPIAware"));
            if (set_process_dpi_aware)
            {
                set_process_dpi_aware();
            }
        }

        SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        impl::open_windows++;
    }

    window::~window() = default;

    bool window::get_resizeable() const
    {
        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    std::string window::get_title() const
    {
        WCHAR title[256];
        GetWindowTextW(m_impl->hwnd, title, 256);

        return impl::narrow(title);
    }

    bool window::get_decorations() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_decorations(); });
        }
        return GetWindowLongW(m_impl->hwnd, GWL_STYLE) & (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    }

    bool window::get_always_on_top() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_always_on_top(); });
        }
        return GetWindowLong(m_impl->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
    }

    std::pair<std::size_t, std::size_t> window::get_size() const
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([this] { return get_size(); });
        }
        RECT rect;
        GetClientRect(m_impl->hwnd, &rect);
        return std::make_pair(rect.right - rect.left, rect.bottom - rect.top);
    }

    std::pair<std::size_t, std::size_t> window::get_min_size() const
    {
        return m_impl->min_size;
    }

    std::pair<std::size_t, std::size_t> window::get_max_size() const
    {
        return m_impl->max_size;
    }

    std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> window::get_background_color() const
    {
        // TODO(Implement)
        return {};
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

    void window::set_resizeable(bool enabled)
    {
        auto current_style = GetWindowLongW(m_impl->hwnd, GWL_STYLE);
        if (enabled)
        {
            current_style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        }
        else
        {
            current_style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        }
        SetWindowLongW(m_impl->hwnd, GWL_STYLE, current_style);
    }

    void window::set_title(const std::string &title)
    {
        SetWindowTextW(m_impl->hwnd, impl::widen(title).c_str());
    }

    void window::set_decorations(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { set_decorations(enabled); });
        }
        auto current_style = GetWindowLongW(m_impl->hwnd, GWL_STYLE);
        if (enabled)
        {
            current_style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        }
        else
        {
            current_style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        }
        SetWindowLongW(m_impl->hwnd, GWL_STYLE, current_style);
    }

    void window::set_always_on_top(bool enabled)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { set_always_on_top(enabled); });
        }
        // NOLINTNEXTLINE
        SetWindowPos(m_impl->hwnd, enabled ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    void window::set_size(std::size_t width, std::size_t height)
    {
        if (!m_impl->is_thread_safe())
        {
            return m_impl->post_safe([=] { set_size(width, height); });
        }
        SetWindowPos(m_impl->hwnd, nullptr, 0, 0, static_cast<int>(width), static_cast<int>(height), SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    void window::set_min_size(std::size_t width, std::size_t height)
    {
        m_impl->min_size = {width, height};
    }

    void window::set_max_size(std::size_t width, std::size_t height)
    {
        m_impl->max_size = {width, height};
    }

    void window::set_background_color(std::size_t r, std::size_t g, std::size_t b, std::size_t a)
    {
        // TODO(Implement)
        (void)r, (void)g, (void)b, (void)a;
    }

    void window::clear(window_event event)
    {
        m_events.clear(event);
    }

    void window::unregister(window_event event, std::size_t id)
    {
        m_events.unregister(event, id);
    }

    template <> std::size_t window::on<window_event::close>(events::get_t<window_event::close> &&callback)
    {
        return m_events.at<window_event::close>().add_callback(std::move(callback));
    }

    template <> std::size_t window::on<window_event::resize>(events::get_t<window_event::resize> &&callback)
    {
        return m_events.at<window_event::resize>().add_callback(std::move(callback));
    }

    void window::run()
    {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
} // namespace saucer