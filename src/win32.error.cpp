#include "win32.error.hpp"

std::error_code saucer::make_error_code(DWORD error)
{
    return {static_cast<int>(error), std::system_category()};
}
