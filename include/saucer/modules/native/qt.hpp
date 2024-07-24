#pragma once

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineProfile>

namespace saucer::native
{
    struct window
    {
        std::unique_ptr<QMainWindow> window;
    };

    struct webview
    {
        std::unique_ptr<QWebEngineProfile> profile;

      public:
        std::unique_ptr<QWebEngineView> web_view;
        std::unique_ptr<QWebEngineView> dev_page;
        std::unique_ptr<QWebEnginePage> web_page;
    };
} // namespace saucer::native
