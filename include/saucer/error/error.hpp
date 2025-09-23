#pragma once

#include <expected>
#include <source_location>

#include <string>
#include <cstdint>

#include <polo/polo.hpp>

namespace saucer
{
    struct error
    {
        struct impl;
        enum class category : std::uint8_t;

      public:
        template <typename T>
        struct of;

      private:
        cr::polo<impl> m_impl;

      public:
        error();
        error(cr::polo<impl>, std::source_location);

      public:
        error(const error &);
        error(error &&) noexcept;

      public:
        ~error();

      public:
        error &operator=(const error &);
        error &operator=(error &&) noexcept;

      public:
        [[nodiscard]] int code() const;
        [[nodiscard]] category type() const;

      public:
        [[nodiscard]] std::string message() const;
        [[nodiscard]] std::source_location location() const;
    };

    enum class error::category : std::uint8_t
    {
        invalid,
        unknown,
        platform,
        contract,
    };

    enum class contract_error : std::uint8_t
    {
        instance_exists,
        required_invalid,
    };

    template <typename T = void>
    using result = std::expected<T, error>;

    template <typename T>
    auto err(T &&, std::source_location = std::source_location::current());
} // namespace saucer

#include "error.inl"
