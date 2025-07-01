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
    struct stable_natives<application>
    {
        QApplication *application;
    };

    template <>
    struct stable_natives<window>
    {
        QMainWindow *window;
    };

    template <>
    struct stable_natives<webview>
    {
        QWebEngineView *webview;
    };

    template <>
    struct stable_natives<permission::request>
    {
#ifdef SAUCER_QT6
        QWebEnginePermission request;
#endif
    };

    template <>
    struct stable_natives<uri>
    {
        QUrl uri;
    };

    template <>
    struct stable_natives<icon>
    {
        QIcon icon;
    };
} // namespace saucer
