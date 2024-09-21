#pragma once

#include <saucer/modules/module.hpp>

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineProfile>

namespace saucer
{
    struct window::impl
    {
        std::unique_ptr<QMainWindow> window;
    };

    struct webview::impl
    {
        std::unique_ptr<QWebEngineProfile> profile;

      public:
        std::unique_ptr<QWebEngineView> web_view;
        std::unique_ptr<QWebEngineView> dev_page;
        std::unique_ptr<QWebEnginePage> web_page;
    };
} // namespace saucer
