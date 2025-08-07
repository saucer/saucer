#pragma once

#include <cstdint>
#include <system_error>

namespace saucer
{
    enum class creation_error : std::uint8_t
    {
        success = 0,
        bad_argument,
        exists_already,
        not_thread_safe,
    };

    [[nodiscard]] std::error_code make_error_code(creation_error);
} // namespace saucer
