#pragma once

#include <string>

namespace saucer
{
    struct error
    {
        virtual ~error() = default;

      public:
        virtual std::string what() = 0;
    };
} // namespace saucer
