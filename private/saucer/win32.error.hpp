#pragma once

#include <system_error>

#include <windows.h>

namespace saucer
{
    [[nodiscard]] std::error_code make_error_code(DWORD);
} // namespace saucer
