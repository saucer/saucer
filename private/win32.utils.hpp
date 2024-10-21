#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace saucer::utils
{
    template <typename T, auto Release>
    class win_handle
    {
        static constexpr T empty = {};

      private:
        T m_handle;

      public:
        win_handle();

      public:
        win_handle(T handle);
        win_handle(win_handle &&other) noexcept;

      public:
        ~win_handle();

      public:
        win_handle &operator=(win_handle &&other) noexcept;

      public:
        [[nodiscard]] const T &get() const;

      public:
        T &reset(T other = empty);
    };

    using string_handle = win_handle<LPWSTR, CoTaskMemFree>;
    using module_handle = utils::win_handle<HMODULE, FreeLibrary>;

    void set_dpi_awareness();
    void set_immersive_dark(HWND, bool);

    [[nodiscard]] WNDPROC overwrite_wndproc(HWND, WNDPROC);

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);

    [[nodiscard]] std::vector<std::uint8_t> read(IStream *stream);
} // namespace saucer::utils

#include "win32.utils.inl"
