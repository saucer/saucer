#include "win32.app.impl.hpp"

#include "win32.utils.hpp"

#include <cassert>

namespace saucer
{
    application::application(const options &opts) : extensible(this), m_pool(opts.threads), m_impl(std::make_unique<impl>())
    {
        m_impl->thread = GetCurrentThreadId();
        m_impl->handle = GetModuleHandleW(nullptr);
        m_impl->id     = utils::widen(opts.id.value());

        m_impl->wnd_class = {
            .lpfnWndProc   = impl::wnd_proc,
            .hInstance     = m_impl->handle,
            .lpszClassName = m_impl->id.c_str(),
        };

        if (!RegisterClassW(&m_impl->wnd_class))
        {
            assert(false && "RegisterClassW() failed");
        }

        m_impl->msg_window = CreateWindowEx(0,                  //
                                            m_impl->id.c_str(), //
                                            L"",                //
                                            0,                  //
                                            0,                  //
                                            0,                  //
                                            0,                  //
                                            0,                  //
                                            HWND_MESSAGE,       //
                                            nullptr,            //
                                            m_impl->handle,     //
                                            nullptr);

        assert(m_impl->msg_window.get() && "Failed to register message only window");

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        Gdiplus::GdiplusStartupInput input{};
        Gdiplus::GdiplusStartup(&m_impl->gdi_token.reset(), &input, nullptr);
    }

    application::~application()
    {
        CoUninitialize();
        UnregisterClassW(m_impl->id.c_str(), m_impl->handle);
    }

    bool application::thread_safe() const
    {
        return m_impl->thread == GetCurrentThreadId();
    }

    void application::post(callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostMessageW(m_impl->msg_window.get(), impl::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
    }

    template <>
    void application::run<true>() const // NOLINT(*-static)
    {
        MSG msg;

        while (GetMessage(&msg, nullptr, 0, 0))
        {
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

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    void application::quit() // NOLINT(*-static)
    {
        PostQuitMessage(0);
    }
} // namespace saucer
