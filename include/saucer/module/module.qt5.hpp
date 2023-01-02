#pragma once
#include "module.hpp"

#include <QMainWindow>
#include <QWebEngineView>

namespace saucer
{
    template <> struct module<backend_type::qt5>::webview_impl
    {
        QWebEngineView *web_view;
    };

    template <> struct module<backend_type::qt5>::window_impl
    {
        QMainWindow *window;
    };
} // namespace saucer