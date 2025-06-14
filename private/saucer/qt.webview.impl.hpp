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
#include <QWebEngineProfile>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestInterceptor>

namespace saucer
{
    struct webview::impl
    {
        class web_class;
        class request_interceptor;

      public:
        std::unique_ptr<QWebEngineProfile> profile;
        std::unique_ptr<request_interceptor> interceptor;

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
        std::unordered_map<std::string, scheme::handler> schemes;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static std::string inject_script();
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

    class webview::impl::request_interceptor : public QWebEngineUrlRequestInterceptor
    {
        Q_OBJECT

      private:
        webview *m_parent;

      public:
        request_interceptor(webview *);

      public:
        void interceptRequest(QWebEngineUrlRequestInfo &) override;
    };
} // namespace saucer
