#pragma once

#include "error.impl.hpp"

#include <windows.h>
#include <gdiplus.h>

namespace saucer
{
    template <>
    struct error::of<DWORD>
    {
        static error operator()(DWORD);
    };

    template <>
    struct error::of<HRESULT>
    {
        static error operator()(HRESULT);
    };

    template <>
    struct error::of<Gdiplus::Status>
    {
        static error operator()(Gdiplus::Status);
    };
} // namespace saucer
