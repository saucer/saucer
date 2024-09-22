#include "win32.app.impl.hpp"

#include <cassert>

namespace saucer
{
    application::application(const options &) : m_impl(std::make_unique<impl>())
    {
        m_impl->thread = GetCurrentThreadId();
        m_impl->handle = GetModuleHandleW(NULL);

        m_impl->wnd_class = {
            .lpfnWndProc   = impl::wnd_proc,
            .hInstance     = m_impl->handle,
            .lpszClassName = L"Saucer",
        };

        if (RegisterClassW(&m_impl->wnd_class))
        {
            return;
        }

        assert(false && "RegisterClassW() failed");
    }

    application::~application()
    {
        UnregisterClassW(L"Saucer", m_impl->handle);
    }

    void application::dispatch(callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostThreadMessageW(m_impl->thread, impl::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
    }

    bool application::thread_safe() const
    {
        return m_impl->thread == GetCurrentThreadId();
    }

    template <>
    void application::run<true>() const // NOLINT(*-static)
    {
        MSG msg;

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            impl::process(msg);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    template <>
    void application::run<false>() const // NOLINT(*-static)
    {
        MSG msg;

        if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            return;
        }

        impl::process(msg);   
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    void application::quit() // NOLINT(*-static)
    {
        PostQuitMessage(0);
    }
} // namespace saucer
