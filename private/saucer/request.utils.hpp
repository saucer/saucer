#pragma once

#include "request.hpp"

#include <array>
#include <string_view>

#include <rebind/utils/name.hpp>

namespace saucer::request::utils
{
    namespace detail
    {
        template <typename T, typename Variant>
        struct contains
        {
            static constexpr auto value = false;
        };

        template <typename T, typename... Ts>
        struct contains<T, std::variant<Ts...>>
        {
            static constexpr auto value = (std::same_as<T, Ts> || ...);
        };

        template <typename T, bool Terminated>
        static constexpr auto tag = []
        {
            constexpr auto name   = rebind::utils::pure_name<T>;
            constexpr auto length = 7 + name.size();

            auto buffer = std::array<char, length + (Terminated ? 1 : 0)>{'s', 'a', 'u', 'c', 'e', 'r', ':'};

            for (auto i = 0uz; name.size() > i; ++i)
            {
                buffer[7 + i] = name[i];
            }

            return buffer;
        }();
    } // namespace detail

    template <typename T, bool Terminated = false>
    static constexpr auto tag = []
    {
        static constexpr auto content = detail::tag<T, Terminated>;
        return std::string_view{content.data(), content.size()};
    }();

    template <typename T>
    static constexpr auto is_request = detail::contains<T, request>::value;
} // namespace saucer::request::utils
