#pragma once

#include "error.hpp"

namespace saucer::errors
{
    class serialize : public error
    {
        std::size_t m_expected;

      public:
        ~serialize() override;

      public:
        std::string what() override;
    };
} // namespace saucer::errors
