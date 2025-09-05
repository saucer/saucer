#pragma once

#include "request.utils.hpp"

#include <array>
#include <concepts>
#include <type_traits>

#include <rebind/utils/name.hpp>

namespace saucer::request::utils
{
    template <typename T, typename Variant>
    struct detail::contains : std::false_type
    {
    };

    template <typename T, typename... Ts>
        requires(std::same_as<T, Ts> || ...)
    struct detail::contains<T, std::variant<Ts...>> : std::true_type
    {
    };

    template <typename T, bool Terminated>
    consteval auto detail::make_tag()
    {
        constexpr auto name   = rebind::utils::pure_name<T>;
        constexpr auto prefix = std::array{'s', 'a', 'u', 'c', 'e', 'r', ':'};

        constexpr auto length = prefix.size() + name.size() + static_cast<std::size_t>(Terminated);
        auto buffer           = std::array<char, length>{};

        for (auto i = 0uz; prefix.size() > i; ++i)
        {
            buffer[i] = prefix[i]; // NOLINT(*-constant-array-index)
        }

        for (auto i = 0uz; name.size() > i; ++i)
        {
            buffer[7 + i] = name[i];
        }

        return buffer;
    }
} // namespace saucer::request::utils
