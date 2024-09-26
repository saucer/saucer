#include "gtk.app.impl.hpp"

#include <ranges>

namespace saucer
{
    std::string application::impl::fix_id(const std::string &id)
    {
        return id                                                                                            //
               | std::views::transform([](const char c) { return std::isalpha(c) ? std::tolower(c) : '_'; }) //
               | std::ranges::to<std::string>();
    }
} // namespace saucer
