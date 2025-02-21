#include "request.utils.hpp"

#include <ranges>
#include <algorithm>

#include <rebind/utils/name.hpp>
#include <rebind/utils/member.hpp>

#include <fmt/core.h>
#include <fmt/xchar.h>

namespace saucer
{
    template <typename T>
    static constexpr auto name = []
    {
        if constexpr (requires {
                          { T::name } -> std::convertible_to<std::string_view>;
                      })
        {
            return T::name;
        }
        else
        {
            return rebind::utils::pure_name<T>;
        }
    }();

    template <typename Message>
    auto make_stub()
    {
        constexpr auto tag     = request::utils::tag<Message>;
        constexpr auto members = rebind::utils::member_names<Message>;

        const auto has_id   = std::ranges::find(members, "id") != members.end();
        const auto filtered = members                                                            //
                              | std::views::filter([](auto &&member) { return member != "id"; }) //
                              | std::ranges::to<std::vector>();

        const auto params     = fmt::format("{}", fmt::join(filtered, ", "));
        const auto invocation = has_id ? fmt::format(R"(send({{ ["{}"]: true, {} }}))", tag, params)
                                       : fmt::format(R"(fire("{}", {{ {} }}))", tag, params);

        return fmt::format(R"({}: ({}) => window.saucer.internal.{})", name<Message>, params, invocation);
    }

    std::string request::stubs()
    {
        const auto stubs = []<auto... Is>(std::index_sequence<Is...>)
        {
            return std::array<std::string, sizeof...(Is)>{make_stub<std::variant_alternative_t<Is, request>>()...};
        }(std::make_index_sequence<std::variant_size_v<request>>());

        return fmt::format("{}", fmt::join(stubs, ",\n\t\t"));
    }
} // namespace saucer
