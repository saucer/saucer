#pragma once
#include <QMainWindow>
#include <QWebEngineView>

namespace saucer::native
{
    struct window
    {
        QMainWindow *window;
    };

    struct webview
    {
        QWebEngineView *web_view;
    };
} // namespace saucer::native
