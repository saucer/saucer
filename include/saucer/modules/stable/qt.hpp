#pragma once

#include <saucer/modules/module.hpp>

#include <saucer/app.hpp>
#include <saucer/window.hpp>
#include <saucer/webview.hpp>

#include <QMainWindow>
#include <QWebEngineView>

namespace saucer
{
    template <>
    struct stable<application>
    {
        QApplication *application;
    };

    template <>
    struct stable<window>
    {
        QMainWindow *window;
    };

    template <>
    struct stable<webview>
    {
        QWebEngineView *webview;
    };
} // namespace saucer
