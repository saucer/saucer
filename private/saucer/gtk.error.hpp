#pragma once

#include "error.impl.hpp"
#include "gtk.utils.hpp"

namespace saucer
{
    template <>
    struct error::of<utils::g_error_ptr> : error::impl
    {
        utils::g_error_ptr value;

      public:
        of(utils::g_error_ptr);

      public:
        of(const of &);

      public:
        [[nodiscard]] int code() const override;
        [[nodiscard]] category type() const override;

      public:
        [[nodiscard]] std::string message() const override;
    };
} // namespace saucer
