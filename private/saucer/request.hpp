#pragma once

#include <string>
#include <variant>

#include <cstdint>
#include <optional>

namespace saucer::request
{
    struct start_resize
    {
        int edge;

      public:
        static constexpr auto name = "startResize";
    };

    struct start_drag
    {
        static constexpr auto name = "startDrag";
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

    struct maximized
    {
        std::uint64_t id;
    };

    struct minimized
    {
        std::uint64_t id;
    };

    using request = std::variant<start_resize, start_drag, maximize, minimize, close, maximized, minimized>;

    [[nodiscard]] std::string stubs();
    [[nodiscard]] std::optional<request> parse(const std::string &);
} // namespace saucer::request
