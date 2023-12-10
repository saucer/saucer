#pragma once

#include "error.hpp"

namespace saucer::errors
{
    class bad_type : public error
    {
        std::size_t m_index;
        std::string m_expected;

      public:
        ~bad_type() override;

      public:
        bad_type(std::size_t index, std::string expected);

      public:
        std::string what() override;
    };
} // namespace saucer::errors
