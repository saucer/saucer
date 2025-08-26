#include "qt.error.hpp"

namespace saucer
{
    error::of<std::string>::of(std::string value) : value(std::move(value)) {}

    int error::of<std::string>::code() const
    {
        return -1;
    }

    error::category error::of<std::string>::type() const
    {
        return category::platform;
    }

    std::string error::of<std::string>::message() const
    {
        return value;
    }
} // namespace saucer
