#pragma once

#include <saucer/modules/module.hpp>

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineProfile>

namespace saucer
{
    struct natives::window_impl
    {
        std::unique_ptr<QMainWindow> window;
    };

    struct natives::webview_impl
    {
        std::unique_ptr<QWebEngineProfile> profile;

      public:
        std::unique_ptr<QWebEngineView> web_view;
        std::unique_ptr<QWebEngineView> dev_page;
        std::unique_ptr<QWebEnginePage> web_page;
    };
} // namespace saucer
