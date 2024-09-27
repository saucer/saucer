#pragma once

#include "../window.hpp"
#include "../webview.hpp"

#include <concepts>

namespace saucer
{
    class smartview_core;

    struct natives
    {
        saucer::window::impl *window;
        saucer::webview::impl *webview;
    };

    template <typename T>
    concept Module = requires(T module) { requires std::constructible_from<T, smartview_core *, natives>; };
} // namespace saucer
