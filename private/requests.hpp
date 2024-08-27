#pragma once

#include <string>
#include <variant>
#include <optional>

namespace saucer::requests
{
    struct resize
    {
        int edge;
    };

    struct drag
    {
    };

    using request = std::variant<resize, drag>;
    std::optional<request> parse(const std::string &);
} // namespace saucer::requests
