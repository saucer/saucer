#include "win32.window.impl.hpp"

#include "win32.app.impl.hpp"

namespace saucer
{
    using native = window::impl::native;
    using event  = window::event;

    void window_flags::apply(HWND hwnd) const
    {
        const auto current = GetWindowLongPtrW(hwnd, GWL_STYLE);
        const auto visible = current & WS_VISIBLE;

        static auto set_flag = [](auto &flags, auto flag, bool enabled)
        {
            flags = enabled ? flags | flag : flags & ~flag;
        };

        auto normal   = standard;
        auto extended = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

        set_flag(normal, WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, resizable);
        set_flag(extended, WS_EX_TRANSPARENT | WS_EX_LAYERED, click_through);

        if (decorations == window::decoration::none)
        {
            normal = 0;
        }

        if (fullscreen)
        {
            set_flag(normal, WS_OVERLAPPEDWINDOW, false);
        }

        if (visible)
        {
            set_flag(normal, WS_VISIBLE, true);
        }

        SetWindowLongPtrW(hwnd, GWL_STYLE, normal);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, extended);
    }

    template <offset T>
    size native::offset(saucer::size size) const
    {
        const auto normal   = GetWindowLongPtrW(hwnd.get(), GWL_STYLE);
        const auto extended = GetWindowLongPtrW(hwnd.get(), GWL_EXSTYLE);

        const auto dpi     = GetDpiForWindow(hwnd.get());
        const auto scaling = static_cast<float>(dpi) / 96.f;

        if (scaling != 1.f)
        {
            size.w = static_cast<int>(static_cast<float>(size.w) * scaling);
            size.h = static_cast<int>(static_cast<float>(size.h) * scaling);
        }

        RECT desired{.left = 0, .top = 0, .right = size.w, .bottom = size.h};
        AdjustWindowRectExForDpi(&desired, normal, false, extended, dpi);

        if (flags.decorations == decoration::partial)
        {
            desired.top = -1; // Remove offset for non-existent titlebar
        }

        const auto offset_x = (desired.right - desired.left) - size.w;
        const auto offset_y = (desired.bottom - desired.top) - size.h;

        if constexpr (T == offset::add)
        {
            size.w += offset_x;
            size.h += offset_y;
        }
        else
        {
            size.w -= offset_x;
            size.h -= offset_y;
        }

        return size;
    }

    template size native::offset<offset::add>(saucer::size) const;
    template size native::offset<offset::sub>(saucer::size) const;

    LRESULT CALLBACK native::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        const auto atom = application::impl::native::ATOM_WINDOW.get();
        auto *self      = reinterpret_cast<impl *>(GetPropW(hwnd, MAKEINTATOM(atom)));

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
            self->events.get<event::decorated>().fire(current);

            break;
        }
        case WM_NCCALCSIZE: {
            if (self->platform->flags.decorations != decoration::partial || !w_param)
            {
                break;
            }

            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(l_param);
            auto *rect   = params->rgrc;

            const auto maximized = self->maximized();
            const auto keep      = !maximized || rect->top >= 0;

            WINDOWINFO info{};
            GetWindowInfo(hwnd, &info);

            auto borders = POINT{
                .x = static_cast<LONG>(info.cxWindowBorders),
                .y = static_cast<LONG>(info.cyWindowBorders),
            };

            // Windows 10 has a bug where the titlebar is rendered, even if `rect->top` is only slightly bigger than the client area.
            // Thus, we do not set an offset here to fix said issue, this sacrifices default resizing from the top though.

            auto offset = 0;

            if (keep)
            {
                offset = (native::windows_build < utils::windows_11_build) ? 0 : 1;
            }
            else
            {
                offset = borders.y;
            }

            rect->top += offset;
            rect->bottom -= borders.y;

            rect->left += borders.x;
            rect->right -= borders.x;

            return 0;
        }
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
            self->events.get<event::focus>().fire(w_param);
            break;
        case WM_SIZE: {
            switch (w_param)
            {
            case SIZE_MAXIMIZED:
                self->events.get<event::maximize>().fire(true);
                break;
            case SIZE_MINIMIZED:
                self->events.get<event::minimize>().fire(true);
                break;
            case SIZE_RESTORED:
                switch (self->platform->prev_state)
                {
                case SIZE_MAXIMIZED:
                    self->events.get<event::maximize>().fire(false);
                    break;
                case SIZE_MINIMIZED:
                    self->events.get<event::minimize>().fire(false);
                    break;
                }
                break;
            }

            self->platform->prev_state = w_param;
            auto [width, height]       = saucer::size{.w = LOWORD(l_param), .h = HIWORD(l_param)};

            self->platform->window_target.Root().Size({static_cast<float>(width), static_cast<float>(height)});
            self->events.get<event::resize>().fire(width, height);

            break;
        }
        case WM_CLOSE: {
            if (self->events.get<event::close>().fire().find(policy::block))
            {
                return 0;
            }

            auto *parent     = self->parent;
            auto *identifier = self->platform->hwnd.get();

            auto *const impl = parent->native<false>()->platform.get();
            auto &instances  = impl->instances;

            self->hide();

            instances.erase(identifier);
            self->events.get<event::closed>().fire();

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
