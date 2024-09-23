#pragma once

#include "webview.hpp"
#include "qt.scheme.impl.hpp"

#include <string>
#include <vector>

#include <string_view>
#include <unordered_map>

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

      public:
        std::vector<script> permanent_scripts;
        std::unordered_map<std::string, scheme::url_scheme_handler> schemes;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;
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
