#pragma once

#include "traits.hpp"

#include <cstdint>

namespace saucer
{
    struct application;
}

namespace saucer::modules
{
    template <>
    class traits<application>
    {
        static constexpr auto on_quit_impl = [](auto &self)
        {
            if constexpr (requires { self.on_quit(); })
            {
                return self.on_quit();
            }
            else
            {
                return false;
            }
        };

      public:
        enum : std::uint8_t
        {
            on_quit,
        };

        using interface = eraser::interface<eraser::method<on_quit, on_quit_impl, bool()>>;
    };
} // namespace saucer::modules
