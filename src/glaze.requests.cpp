#include "requests.hpp"

#include <glaze/glaze.hpp>

template <>
struct glz::meta<saucer::requests::resize>
{
    using T                     = saucer::requests::resize;
    static constexpr auto value = object( //
        "saucer:resize", skip{},          //
        "edge", &T::edge                  //
    );
};

template <>
struct glz::meta<saucer::requests::maximize>
{
    using T                     = saucer::requests::maximize;
    static constexpr auto value = object( //
        "saucer:maximize", skip{},        //
        "value", &T::value                //
    );
};

template <>
struct glz::meta<saucer::requests::minimize>
{
    using T                     = saucer::requests::minimize;
    static constexpr auto value = object( //
        "saucer:minimize", skip{},        //
        "value", &T::value                //
    );
};

template <>
struct glz::meta<saucer::requests::drag>
{
    using T                     = saucer::requests::drag;
    static constexpr auto value = object( //
        "saucer:drag", skip{}             //
    );
};

template <>
struct glz::meta<saucer::requests::maximized>
{
    using T                     = saucer::requests::maximized;
    static constexpr auto value = object( //
        "saucer:maximized", skip{},       //
        "id", &T::id                      //
    );
};

template <>
struct glz::meta<saucer::requests::minimized>
{
    using T                     = saucer::requests::minimized;
    static constexpr auto value = object( //
        "saucer:minimized", skip{},       //
        "id", &T::id                      //
    );
};

namespace saucer
{
    static constexpr auto opts = glz::opts{.error_on_unknown_keys = true, .error_on_missing_keys = true};

    std::optional<requests::request> requests::parse(const std::string &data)
    {
        requests::request rtn{};

        if (auto err = glz::read<opts>(rtn, data); err)
        {
            return std::nullopt;
        }

        return rtn;
    }
} // namespace saucer
