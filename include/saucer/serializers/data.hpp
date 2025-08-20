#pragma once

#include <string>
#include <cstddef>

namespace saucer
{
    struct function_data
    {
        std::size_t id;
        std::string name;

      public:
        virtual ~function_data() = default;
    };

    struct result_data
    {
        std::size_t id;

      public:
        virtual ~result_data() = default;
    };
} // namespace saucer
