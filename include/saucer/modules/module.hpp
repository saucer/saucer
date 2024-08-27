#pragma once

#include <concepts>

namespace saucer
{
    class smartview_core;

    struct natives
    {
        struct window_impl;
        struct webview_impl;

      public:
        window_impl *window;
        webview_impl *webview;
    };

    template <typename T>
    concept Module = requires(T module) { requires std::constructible_from<T, smartview_core *, natives>; };
} // namespace saucer
