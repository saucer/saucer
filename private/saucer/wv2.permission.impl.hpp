#pragma once

#include <saucer/permission.hpp>

#include <wrl.h>
#include <WebView2.h>

namespace saucer::permission
{
    using Microsoft::WRL::ComPtr;

    struct request::impl
    {
        ComPtr<ICoreWebView2PermissionRequestedEventArgs> request;
        ComPtr<ICoreWebView2Deferral> deferral;
    };
} // namespace saucer::permission
