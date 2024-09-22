#include "win32.app.impl.hpp"

namespace saucer
{
    void application::impl::process(MSG message)
    {
        if (message.message != WM_SAFE_CALL)
        {
            return;
        }

        delete reinterpret_cast<safe_message *>(message.lParam);
    }

    LRESULT CALLBACK application::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    safe_message::safe_message(callback_t callback) : m_callback(std::move(callback)) {}

    safe_message::~safe_message()
    {
        std::invoke(m_callback);
    }
} // namespace saucer
