#include "desktop.hpp"

#include "win32.utils.hpp"

#include <windows.h>

namespace saucer
{
    void desktop::open(const std::string &uri)
    {
        ShellExecuteW(nullptr, L"open", utils::widen(uri).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
} // namespace saucer
