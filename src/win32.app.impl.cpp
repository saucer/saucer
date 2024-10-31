#include "win32.app.impl.hpp"

namespace saucer
{
    LRESULT CALLBACK application::impl::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        if (msg != WM_SAFE_CALL)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        delete reinterpret_cast<safe_message *>(l_param);
        return 0;
    }

    safe_message::safe_message(callback_t callback) : m_callback(std::move(callback)) {}

    safe_message::~safe_message()
    {
        std::invoke(m_callback);
    }
} // namespace saucer
