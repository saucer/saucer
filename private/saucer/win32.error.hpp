#pragma once

#include "error.impl.hpp"

#include <windows.h>
#include <gdiplus.h>

namespace saucer
{
    template <>
    struct error::of<DWORD> : error::of<std::error_code>
    {
        of(DWORD);
    };

    template <>
    struct error::of<HRESULT> : error::of<DWORD>
    {
        of(HRESULT);
    };

    template <>
    struct error::of<Gdiplus::Status> : error::impl
    {
        Gdiplus::Status value;

      public:
        of(Gdiplus::Status);

      public:
        [[nodiscard]] int code() const override;
        [[nodiscard]] category type() const override;

      public:
        [[nodiscard]] std::string message() const override;
    };
} // namespace saucer
