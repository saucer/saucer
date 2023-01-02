#pragma once
#include "module.hpp"

namespace saucer
{
    template <backend_type Backend>
    module<Backend>::module(smartview &smartview, webview_impl *webview, window_impl *window)
        : m_smartview(smartview), m_webview_impl(webview), m_window_impl(window)
    {
    }

    template <backend_type Backend> module<Backend>::~module() = default;
} // namespace saucer