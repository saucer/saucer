#pragma once

#include "scheme.hpp"

#include <wrl.h>
#include <WebView2.h>

namespace saucer
{
    using Microsoft::WRL::ComPtr;

    struct request::impl
    {
        ICoreWebView2WebResourceRequestedEventArgs *args;
        ComPtr<ICoreWebView2WebResourceRequest> request;
    };
} // namespace saucer
