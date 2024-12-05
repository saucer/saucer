#pragma once

#include "handle.hpp"

#include <string>
#include <cstdint>

#include <array>
#include <vector>

#include <windows.h>

namespace saucer::utils
{
    using string_handle = utils::handle<LPWSTR, CoTaskMemFree>;
    using module_handle = utils::handle<HMODULE, FreeLibrary>;
    using window_handle = utils::handle<HWND, DestroyWindow>;

    void set_dpi_awareness();
    void set_immersive_dark(HWND, bool);
    void extend_frame(HWND, std::array<int, 4>);

    [[nodiscard]] WNDPROC overwrite_wndproc(HWND, WNDPROC);

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *stream);
} // namespace saucer::utils
