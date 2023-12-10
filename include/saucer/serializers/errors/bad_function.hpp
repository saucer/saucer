#pragma once

#include "error.hpp"

namespace saucer::errors
{
    class bad_function : public error
    {
        std::string m_function;

      public:
        ~bad_function() override;

      public:
        bad_function(std::string function);

      public:
        std::string what() override;
    };
} // namespace saucer::errors
