#include "gtk.app.impl.hpp"

#include <ranges>

namespace saucer
{
    screen application::impl::convert(GdkMonitor *monitor)
    {
        const auto *model = gdk_monitor_get_model(monitor);

        GdkRectangle rect{};
        gdk_monitor_get_geometry(monitor, &rect);

        return {
            .name     = model ? model : "",
            .size     = {rect.width, rect.height},
            .position = {rect.x, rect.y},
        };
    }

    std::string application::impl::fix_id(const std::string &id)
    {
        return id                                                                                            //
               | std::views::transform([](const char c) { return std::isalpha(c) ? std::tolower(c) : '_'; }) //
               | std::ranges::to<std::string>();
    }
} // namespace saucer
