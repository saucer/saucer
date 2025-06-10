#include "win32.app.impl.hpp"

#include <cassert>

namespace saucer
{
    application::application(impl data) : extensible(this), m_impl(std::make_unique<impl>(std::move(data))) {}

    application::~application()
    {
        UnregisterClassW(m_impl->id.c_str(), m_impl->handle);
        CoUninitialize();
    }

    bool application::thread_safe() const
    {
        return m_impl->thread == GetCurrentThreadId();
    }

    coco::future<void> application::finish()
    {
        return std::move(m_impl->future);
    }

    std::vector<screen> application::screens() const
    {
        if (!thread_safe())
        {
            return dispatch([this] { return screens(); });
        }

        std::vector<screen> rtn{};
        EnumDisplayMonitors(nullptr, nullptr, impl::enum_monitor, reinterpret_cast<LPARAM>(&rtn));

        return rtn;
    }

    void application::post(post_callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostMessageW(m_impl->msg_window.get(), impl::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
    }

    int application::run(callback_t callback) // NOLINT(*-static)
    {
        static bool once{false};

        if (!thread_safe())
        {
            assert(false && "saucer::application::run() may only be called from the thread it was created in");
            return -1;
        }

        if (once)
        {
            assert(false && "saucer::application::run() may only be called once");
            return -1;
        }

        auto promise = coco::promise<void>{};

        once           = true;
        m_impl->future = promise.get_future();

        post([this, callback = std::move(callback)] mutable { std::invoke(callback, this); });

        MSG msg{};

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        promise.set_value();

        return 0;
    }

    void application::quit() // NOLINT(*-static)
    {
        PostQuitMessage(0);
    }

    std::optional<application> application::create(const options &opts)
    {
        static bool once{false};

        if (once)
        {
            assert(false && "saucer::application may only be created once");
            return std::nullopt;
        }

        once = true;

        impl rtn;

        rtn.thread                     = GetCurrentThreadId();
        rtn.handle                     = GetModuleHandleW(nullptr);
        rtn.id                         = utils::widen(opts.id.value());
        rtn.quit_on_last_window_closed = opts.quit_on_last_window_closed;
        rtn.wnd_class = WNDCLASSW{.lpfnWndProc = impl::wnd_proc, .hInstance = rtn.handle, .lpszClassName = rtn.id.c_str()};

        if (!RegisterClassW(&rtn.wnd_class))
        {
            assert(false && "RegisterClassW() failed");
            return std::nullopt;
        }

        rtn.msg_window = CreateWindowEx(0,              //
                                        rtn.id.c_str(), //
                                        L"",            //
                                        0,              //
                                        0,              //
                                        0,              //
                                        0,              //
                                        0,              //
                                        HWND_MESSAGE,   //
                                        nullptr,        //
                                        rtn.handle,     //
                                        nullptr);

        if (!rtn.msg_window.get())
        {
            assert(false && "CreateWindowEx() failed");
            return std::nullopt;
        }

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        Gdiplus::GdiplusStartupInput input{};
        Gdiplus::GdiplusStartup(&rtn.gdi_token.reset(), &input, nullptr);

        return rtn;
    }
} // namespace saucer
