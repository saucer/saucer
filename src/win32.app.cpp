#include "win32.app.impl.hpp"
#include "win32.error.hpp"

namespace saucer
{
    using impl = application::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        auto controller = utils::create_dispatch_controller();

        if (!controller.has_value())
        {
            return err(controller);
        }

        platform = std::make_unique<native>();

        platform->id                         = utils::widen(opts.id.value());
        platform->handle                     = GetModuleHandleW(nullptr);
        platform->dispatch_controller        = std::move(controller.value());
        platform->quit_on_last_window_closed = opts.quit_on_last_window_closed;

        platform->wnd_class = WNDCLASSW{
            .lpfnWndProc   = native::wnd_proc,
            .hInstance     = platform->handle,
            .lpszClassName = platform->id.c_str(),
        };

        if (!RegisterClassW(&platform->wnd_class))
        {
            return err(make_error_code(GetLastError()));
        }

        platform->msg_window = CreateWindowEx(0,                    //
                                              platform->id.c_str(), //
                                              L"",                  //
                                              0,                    //
                                              0,                    //
                                              0,                    //
                                              0,                    //
                                              0,                    //
                                              HWND_MESSAGE,         //
                                              nullptr,              //
                                              platform->handle,     //
                                              nullptr);

        if (!platform->msg_window.get())
        {
            return err(make_error_code(GetLastError()));
        }

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        Gdiplus::GdiplusStartupInput input{};
        Gdiplus::GdiplusStartup(&platform->gdi_token.reset(), &input, nullptr);

        return {};
    }

    impl::~impl()
    {
        UnregisterClassW(platform->id.c_str(), platform->handle);
        CoUninitialize();
    }

    std::vector<screen> impl::screens() const // NOLINT(*-static)
    {
        std::vector<screen> rtn{};
        EnumDisplayMonitors(nullptr, nullptr, native::enum_monitor, reinterpret_cast<LPARAM>(&rtn));

        return rtn;
    }

    void application::post(post_callback_t callback) const
    {
        auto *message = new safe_message{std::move(callback)};
        PostMessageW(m_impl->platform->msg_window.get(), impl::native::WM_SAFE_CALL, 0, reinterpret_cast<LPARAM>(message));
    }

    int impl::run(application *self, callback_t callback) // NOLINT(*-static)
    {
        auto promise = coco::promise<void>{};
        finish       = promise.get_future();

        self->post([&callback, self] { std::invoke(callback, self); });

        MSG msg{};

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        promise.set_value();

        return 0;
    }

    void impl::quit() // NOLINT(*-static)
    {
        PostQuitMessage(0);
    }
} // namespace saucer
