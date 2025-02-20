#pragma once

#include "request.hpp"

#include <array>
#include <string_view>

#include <fmt/compile.h>
#include <rebind/utils/name.hpp>

namespace saucer::request::utils
{
    namespace impl
    {
        template <typename T, typename Variant>
        struct contains
        {
            static constexpr auto value = false;
        };

        template <typename T, typename... Ts>
        struct contains<T, std::variant<Ts...>>
        {
            static constexpr auto value = (std::is_same_v<T, Ts> || ...);
        };

        template <typename T, bool Terminated>
        static constexpr auto tag = []
        {
            constexpr auto name   = rebind::utils::pure_name<T>;
            constexpr auto length = 7 + name.size();

            std::array<char, length + (Terminated ? 1 : 0)> buffer{};
            fmt::format_to(buffer.data(), FMT_COMPILE("saucer:{}"), name);

            return buffer;
        }();
    } // namespace impl

    template <typename T, bool Terminated = false>
    static constexpr auto tag = std::string_view{impl::tag<T, Terminated>.data(), impl::tag<T, Terminated>.size()};

    template <typename T>
    static constexpr auto is_request = impl::contains<T, request>::value;
} // namespace saucer::request::utils
