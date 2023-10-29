#pragma once

#include <string>
#include <variant>
#include <glaze/glaze.hpp>

namespace saucer
{
    struct resize_request
    {
        int edge;
    };

    struct drag_request
    {
    };

    using request = std::variant<resize_request, drag_request>;
} // namespace saucer

template <>
struct glz::meta<saucer::resize_request>
{
    using T                     = saucer::resize_request;
    static constexpr auto value = object( //
        "saucer:resize", skip{},          //
        "edge", &T::edge                  //
    );
};

template <>
struct glz::meta<saucer::drag_request>
{
    using T                     = saucer::drag_request;
    static constexpr auto value = object( //
        "saucer:drag", skip{}             //
    );
};
