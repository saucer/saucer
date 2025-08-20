#include "gtk.app.impl.hpp"

#include <ranges>

namespace saucer
{
    using native = application::impl::native;

    void native::iteration()
    {
        auto *const context = g_main_context_default();
        g_main_context_iteration(context, false);
    }

    screen native::convert(GdkMonitor *monitor)
    {
        const auto *model = gdk_monitor_get_model(monitor);

        GdkRectangle rect{};
        gdk_monitor_get_geometry(monitor, &rect);

        return {
            .name     = model ? model : "",
            .size     = {.w = rect.width, .h = rect.height},
            .position = {.x = rect.x, .y = rect.y},
        };
    }

    std::string native::fix_id(const std::string &id)
    {
        return id                                                                                            //
               | std::views::transform([](const char c) { return std::isalpha(c) ? std::tolower(c) : '_'; }) //
               | std::ranges::to<std::string>();
    }
} // namespace saucer
