#pragma once

#include "request.hpp"

#include <string_view>

namespace saucer::request::utils
{
    namespace detail
    {
        template <typename T, typename Variant>
        struct contains;

        template <typename T, bool Terminated>
        consteval auto make_tag();
    } // namespace detail

    template <typename T, bool Terminated = false>
    static constexpr auto raw_tag = []
    {
        return detail::make_tag<T, Terminated>();
    }();

    template <typename T, bool Terminated = false>
    static constexpr auto tag = []
    {
        static constexpr auto raw = raw_tag<T, Terminated>;
        return std::string_view{raw.data(), raw.size()};
    }();

    template <typename T>
    static constexpr auto is_request = detail::contains<T, request>::value;
} // namespace saucer::request::utils

#include "request.utils.inl"
