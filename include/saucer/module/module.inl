#pragma once
#include "module.hpp"

namespace saucer
{
    template <typename T> struct is_implementation_module
    {
      private:
        static auto test(...) -> std::uint8_t;
        template <typename O> static auto test(O *) -> decltype(std::declval<O>().template load<backend>(nullptr, nullptr), std::uint16_t{});

      public:
        static constexpr bool value = sizeof(test(static_cast<T *>(0))) == sizeof(std::uint16_t);
    };
} // namespace saucer