#pragma once

#include <cstdint>
#include <system_error>

#include <expected>
#include <source_location>

namespace saucer
{
    enum class contract_error : std::uint8_t
    {
        success          = 0,
        instance_exists  = 1,
        not_main_thread  = 2,
        required_invalid = 3,
    };

    [[nodiscard]] const std::error_category &contract_category();
    [[nodiscard]] std::error_code make_error_code(contract_error);

    struct error
    {
        std::error_code error_code;
        std::source_location location;
    };

    template <typename T = void>
    using result = std::expected<T, error>;

    template <typename T>
    auto err(std::expected<T, error> &);

    template <typename T>
    auto err(T &&, std::source_location = std::source_location::current());
} // namespace saucer

#include "error.inl"
