#pragma once

#include <saucer/error/error.hpp>

#include <system_error>

namespace saucer
{
    struct error::impl
    {
        std::source_location location;

      public:
        virtual ~impl() = default;

      public:
        [[nodiscard]] virtual int code() const;
        [[nodiscard]] virtual category type() const;

      public:
        [[nodiscard]] virtual std::string message() const;
    };

    template <>
    struct error::of<std::error_code> : error::impl
    {
        std::error_code value;

      public:
        of(std::error_code);

      public:
        [[nodiscard]] int code() const override;
        [[nodiscard]] category type() const override;

      public:
        [[nodiscard]] std::string message() const override;
    };

    template <>
    struct error::of<std::errc> : error::of<std::error_code>
    {
        of(std::errc);
    };

    template <>
    struct error::of<contract_error> : error::impl
    {
        contract_error value;

      public:
        of(contract_error);

      public:
        [[nodiscard]] int code() const override;
        [[nodiscard]] category type() const override;

      public:
        [[nodiscard]] std::string message() const override;
    };
} // namespace saucer
