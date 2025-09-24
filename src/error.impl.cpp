#include "error.impl.hpp"

#include <utility>

namespace saucer
{
    int error::impl::code() const
    {
        return -1;
    }

    error::category error::impl::type() const
    {
        return category::invalid;
    }

    std::string error::impl::message() const
    {
        return {};
    }

    error::of<std::error_code>::of(std::error_code value) : value(value) {}

    int error::of<std::error_code>::code() const
    {
        return value.value();
    }

    error::category error::of<std::error_code>::type() const
    {
        return category::platform;
    }

    std::string error::of<std::error_code>::message() const
    {
        return value.message();
    }

    error::of<std::errc>::of(std::errc value) : of<std::error_code>(std::make_error_code(value)) {}

    error::of<contract_error>::of(contract_error value) : value(value) {}

    int error::of<contract_error>::code() const
    {
        return std::to_underlying(value);
    }

    error::category error::of<contract_error>::type() const
    {
        return category::contract;
    }

    std::string error::of<contract_error>::message() const
    {
        switch (value)
        {
            using enum contract_error;

        case instance_exists:
            return "instance already exists";

        case required_invalid:
            return "required argument was invalid";
        }

        std::unreachable();
    }
} // namespace saucer
