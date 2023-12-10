#pragma once

#include "error.hpp"

namespace saucer::errors
{
    class serialize : public error
    {
      public:
        ~serialize() override;

      public:
        std::string what() override;
    };
} // namespace saucer::errors
