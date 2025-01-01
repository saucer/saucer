#include "requests.hpp"

#include <rfl/json.hpp>

namespace rfl
{
    template <>
    struct Reflector<saucer::requests::resize>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:resize", bool> tag;
            int edge;
        };

        static saucer::requests::resize to(const ReflType &v) noexcept
        {
            return {v.edge};
        }
    };

    template <>
    struct Reflector<saucer::requests::drag>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:drag", bool> tag;
        };

        static saucer::requests::drag to(const ReflType &) noexcept
        {
            return {};
        }
    };
} // namespace rfl

namespace saucer
{
    std::optional<requests::request> requests::parse(const std::string &data)
    {
        auto result = rfl::json::read<requests::request>(data);

        if (!result)
        {
            return std::nullopt;
        }

        return result.value();
    }
} // namespace saucer
