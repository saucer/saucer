#pragma once

#include <string>

namespace saucer
{
    enum class error_code
    {
        unknown,
        rejected,
        type_mismatch,
        unknown_function,
    };

    struct error
    {
        error_code ec;
        std::string message;
    };
} // namespace saucer
