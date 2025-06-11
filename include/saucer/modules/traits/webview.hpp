#pragma once

#include "traits.hpp"

#include <cstdint>
#include <string_view>

namespace saucer
{
    struct webview;
}

namespace saucer::modules
{
    template <>
    class traits<webview>
    {
        static constexpr auto on_message_impl = [](auto &self, const auto &message)
        {
            if constexpr (requires { self.on_message(message); })
            {
                return self.on_message(message);
            }
            else
            {
                return false;
            }
        };

      public:
        enum : std::uint8_t
        {
            on_message,
        };

        using interface = eraser::interface<eraser::method<on_message, on_message_impl, bool(std::string_view)>>;
    };
} // namespace saucer::modules
