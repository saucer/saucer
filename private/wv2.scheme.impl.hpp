#pragma once

#include "scheme.hpp"

#include <wrl.h>
#include <WebView2.h>

namespace saucer::scheme
{
    using Microsoft::WRL::ComPtr;

    struct request::impl
    {
        ICoreWebView2WebResourceRequestedEventArgs *args;
        ComPtr<ICoreWebView2WebResourceRequest> request;
        ComPtr<IStream> body;
    };
} // namespace saucer::scheme
