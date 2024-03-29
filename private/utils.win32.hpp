#pragma once

#include <string>

namespace saucer::utils
{
    void throw_error(const std::string &);

    void set_dpi_awareness();

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);
} // namespace saucer::utils
