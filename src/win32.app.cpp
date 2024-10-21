#include "win32.app.impl.hpp"

#include "win32.utils.hpp"

#include <cassert>

namespace saucer
{
    application::application(const options &options) : m_impl(std::make_unique<impl>())
    {
        m_impl->thread = GetCurrentThreadId();
        m_impl->handle = GetModuleHandleW(nullptr);
        m_impl->id     = utils::widen(options.id.value());

        m_impl->wnd_class = {
            .lpfnWndProc   = impl::wnd_proc,
            .hInstance     = m_impl->handle,
            .lpszClassName = m_impl->id.c_str(),
        };

        if (!RegisterClassW(&m_impl->wnd_class))
        {
            assert(false && "RegisterClassW() failed");
        }

        Gdiplus::GdiplusStartupInput input{};
        Gdiplus::GdiplusStartup(&m_impl->gdi_token.reset(), &input, nullptr);
    }

    application::~application()
    {
        UnregisterClassW(m_impl->id.c_str(), m_impl->handle);
    }

    bool application::thread_safe() const
    {
        return m_impl->thread == GetCurrentThreadId();
    }

    void application::post(callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostThreadMessageW(m_impl->thread, impl::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
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
