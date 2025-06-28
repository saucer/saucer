#pragma once

#include "uri.hpp"

#include <windows.h>
#include <wininet.h>

namespace saucer
{
    struct uri::impl
    {
        std::wstring url;
        URL_COMPONENTSW components;
    };
} // namespace saucer
