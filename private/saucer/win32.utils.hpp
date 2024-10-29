#pragma once

#include <cstdint>

#include <string>
#include <vector>

#include <windows.h>
#include <saucer/utils/handle.hpp>

namespace saucer::utils
{
    using string_handle = utils::handle<LPWSTR, CoTaskMemFree>;
    using module_handle = utils::handle<HMODULE, FreeLibrary>;

    void set_dpi_awareness();
    void set_immersive_dark(HWND, bool);

    [[nodiscard]] WNDPROC overwrite_wndproc(HWND, WNDPROC);

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *stream);
} // namespace saucer::utils
