#pragma once
#include <string>

namespace saucer
{
    std::string last_error();
    void set_dpi_awareness();

    std::string narrow(const std::wstring &wide);
    std::wstring widen(const std::string &narrow);
} // namespace saucer