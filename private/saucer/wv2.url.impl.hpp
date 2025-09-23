#pragma once

#include <saucer/url.hpp>

#include <windows.h>
#include <wininet.h>

namespace saucer
{
    struct url::impl
    {
        std::wstring url;
        URL_COMPONENTSW components;
    };
} // namespace saucer
