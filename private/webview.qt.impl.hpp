#pragma once

#include "webview.hpp"
#include "scheme.qt.impl.hpp"

#include <string>
#include <vector>

#include <QMetaObject>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebEngineUrlSchemeHandler>

namespace saucer
{
    struct webview::impl
    {
        class web_class;

      public:
        std::unique_ptr<QWebEngineProfile> profile;

      public:
        std::unique_ptr<QWebEngineView> web_view;
        std::unique_ptr<QWebEngineView> dev_page;
        std::unique_ptr<QWebEnginePage> web_page;

      public:
        std::unique_ptr<QWebChannel> channel;
        std::unique_ptr<QObject> channel_obj;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;
        std::map<std::string, url_scheme_handler> schemes;

      public:
        QMetaObject::Connection url_changed;
        QMetaObject::Connection load_finished;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string ready_script;
        static const std::string inject_script;
    };

    class webview::impl::web_class : public QObject
    {
        Q_OBJECT

      private:
        webview *m_parent;

      public:
        web_class(webview *);

      public slots:
        void on_message(const QString &);
    };
} // namespace saucer
