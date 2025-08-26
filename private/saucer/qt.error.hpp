#pragma once

#include "error.impl.hpp"

namespace saucer
{
    template <>
    struct error::of<std::string> : error::impl
    {
        std::string value;

      public:
        of(std::string);

      public:
        [[nodiscard]] int code() const override;
        [[nodiscard]] category type() const override;

      public:
        [[nodiscard]] std::string message() const override;
    };
} // namespace saucer
