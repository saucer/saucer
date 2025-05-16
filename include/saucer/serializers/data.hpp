#pragma once

#include <string>
#include <cstdint>

namespace saucer
{
    struct function_data
    {
        std::uint64_t id;
        std::string name;

      public:
        virtual ~function_data() = default;
    };

    struct result_data
    {
        std::uint64_t id;

      public:
        virtual ~result_data() = default;
    };
} // namespace saucer
