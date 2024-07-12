#pragma once

#include <string>

namespace saucer
{
    enum class error_code
    {
        unknown,
        type_mismatch,
        unknown_function,
    };

    struct error
    {
        error_code error;
        std::string message;
    };
} // namespace saucer
