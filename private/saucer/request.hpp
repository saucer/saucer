#pragma once

#include <string>
#include <variant>

#include <cstdint>
#include <optional>

namespace saucer::request
{
    struct resize
    {
        int edge;
    };

    struct maximize
    {
        bool value;
    };

    struct minimize
    {
        bool value;
    };

    struct close
    {
    };

    struct drag
    {
    };

    struct maximized
    {
        std::uint64_t id;
    };

    struct minimized
    {
        std::uint64_t id;
    };

    using request = std::variant<resize, maximize, minimize, close, drag, maximized, minimized>;

    std::optional<request> parse(const std::string &);
} // namespace saucer::request
