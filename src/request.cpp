#include "request.utils.hpp"

#include <format>
#include <ranges>

#include <algorithm>

#include <rebind/utils/name.hpp>
#include <rebind/utils/member.hpp>

namespace saucer
{
    template <typename T>
    concept has_name = requires() {
        { T::name } -> std::convertible_to<std::string_view>;
    };

    template <typename T>
    static constexpr auto name = []
    {
        if constexpr (has_name<T>)
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
        constexpr auto tag      = request::utils::tag<Message>;
        constexpr auto members  = rebind::utils::member_names<Message>;
        constexpr auto contains = std::ranges::contains(members, "id");

        auto filtered = members //
                        | std::views::filter([](auto &&member) { return member != "id"; });

        auto params = filtered                     //
                      | std::views::join_with(',') //
                      | std::ranges::to<std::string>();

        const auto invocation = contains //
                                    ? std::format(R"(send({{ ["{}"]: true, {} }}))", tag, params)
                                    : std::format(R"(fire("{}", {{ {} }}))", tag, params);

        return std::format(R"({}: ({}) => window.saucer.internal.{})", name<Message>, params, invocation);
    }

    std::string request::stubs()
    {
        const auto stubs = []<auto... Is>(std::index_sequence<Is...>)
        {
            return std::array<std::string, sizeof...(Is)>{make_stub<std::variant_alternative_t<Is, request>>()...};
        }(std::make_index_sequence<std::variant_size_v<request>>());

        return stubs                                           //
               | std::views::join_with(std::string{",\n\t\t"}) //
               | std::ranges::to<std::string>();
    }
} // namespace saucer
