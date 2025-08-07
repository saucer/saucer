#pragma once

#include <cstdint>
#include <system_error>

namespace saucer
{
    enum class qt_error : std::uint8_t
    {
        success = 0,
        no_web_channel,
    };

    [[nodiscard]] std::error_code make_error_code(qt_error);
} // namespace saucer
