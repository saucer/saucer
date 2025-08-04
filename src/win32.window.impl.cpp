#include "win32.window.impl.hpp"

#include "win32.app.impl.hpp"

namespace saucer
{
    using native = window::impl::native;
    using event  = window::event;

    void native::set_style(HWND hwnd, long style)
    {
        auto current = GetWindowLongPtr(hwnd, GWL_STYLE);

        if (current & WS_VISIBLE)
        {
            style |= WS_VISIBLE;
        }

        SetWindowLongPtr(hwnd, GWL_STYLE, style);
    }

    LRESULT CALLBACK native::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        auto *self = reinterpret_cast<window::impl *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!self)
        {
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }

        switch (msg)
        {
        case WM_STYLECHANGED: {
            auto &prev         = self->platform->prev_decoration;
            const auto current = self->decorations();

            if (prev.has_value() && prev.value() == current)
            {
                break;
            }

            prev.emplace(current);
            self->events->get<event::decorated>().fire(current);

            break;
        }
        case WM_NCCALCSIZE:
            if (!self->platform->titlebar)
            {
                auto *const rect = w_param ? &reinterpret_cast<NCCALCSIZE_PARAMS *>(l_param)->rgrc[0] //
                                           : reinterpret_cast<RECT *>(l_param);

                const auto maximized = self->maximized();
                const auto keep      = !maximized || rect->top >= 0;

                WINDOWINFO info{};
                GetWindowInfo(hwnd, &info);

                rect->top += keep ? (maximized ? 0 : 1) : static_cast<LONG>(info.cxWindowBorders);
                rect->bottom -= static_cast<LONG>(info.cyWindowBorders);

                rect->left += static_cast<LONG>(info.cxWindowBorders);
                rect->right -= static_cast<LONG>(info.cxWindowBorders);

                return 0;
            }
            break;
        case WM_GETMINMAXINFO: {
            auto *info = reinterpret_cast<MINMAXINFO *>(l_param);

            if (auto min_size = self->platform->min_size; min_size)
            {
                auto [min_x, min_y]  = min_size.value();
                info->ptMinTrackSize = {.x = min_x, .y = min_y};
            }

            if (auto max_size = self->platform->max_size; max_size)
            {
                auto [max_x, max_y]  = max_size.value();
                info->ptMaxTrackSize = {.x = max_x, .y = max_y};
            }

            break;
        }
        case WM_NCACTIVATE:
            self->events->get<event::focus>().fire(w_param);
            break;
        case WM_SIZE: {
            switch (w_param)
            {
            case SIZE_MAXIMIZED:
                self->events->get<event::maximize>().fire(true);
                break;
            case SIZE_MINIMIZED:
                self->events->get<event::minimize>().fire(true);
                break;
            case SIZE_RESTORED:
                switch (self->platform->prev_state)
                {
                case SIZE_MAXIMIZED:
                    self->events->get<event::maximize>().fire(false);
                    break;
                case SIZE_MINIMIZED:
                    self->events->get<event::minimize>().fire(false);
                    break;
                }
                break;
            }

            self->platform->prev_state = w_param;

            auto [width, height] = self->size();
            self->events->get<event::resize>().fire(width, height);

            break;
        }
        case WM_CLOSE: {
            if (self->events->get<event::close>().fire().find(policy::block))
            {
                return 0;
            }

            auto *parent     = self->parent;
            auto *identifier = self->platform->hwnd.get();

            auto *const impl = parent->native<false>()->platform.get();
            auto &instances  = impl->instances;

            self->hide();

            instances.erase(identifier);
            self->events->get<event::closed>().fire();

            if (!impl->quit_on_last_window_closed)
            {
                return 0;
            }

            if (!std::ranges::any_of(instances | std::views::values, std::identity{}))
            {
                parent->quit();
            }

            return 0;
        }
        }

        return CallWindowProcW(self->platform->hook.original(), hwnd, msg, w_param, l_param);
    }
} // namespace saucer
