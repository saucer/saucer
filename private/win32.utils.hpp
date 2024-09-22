#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace saucer::utils
{
    void set_dpi_awareness();
    void set_immersive_dark(HWND, bool);

    [[nodiscard]] WNDPROC overwrite_wndproc(HWND, WNDPROC);

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *stream);
} // namespace saucer::utils
