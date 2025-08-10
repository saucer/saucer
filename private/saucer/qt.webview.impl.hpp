#pragma once

#include "webview.impl.hpp"
#include "qt.scheme.impl.hpp"

#include <string>
#include <vector>

#include <unordered_map>

#include <QMetaObject>
#include <QWebChannel>

#include <QWebEngineView>
#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestInterceptor>

namespace saucer
{
    struct web_class;
    struct request_interceptor;

    struct qt_script
    {
        std::string id;
        load_time time;
        bool clearable;
    };

    struct webview::impl::native
    {
        std::unique_ptr<QWebEngineProfile> profile;

      public:
        std::unique_ptr<QWebEngineView> web_view;
        std::unique_ptr<QWebEnginePage> web_page;
        std::unique_ptr<QWebEngineView> dev_page;

      public:
        std::unique_ptr<QWebChannel> channel;
        std::unique_ptr<QObject> channel_obj;

      public:
        std::uint64_t id_counter{0};
        std::unordered_map<std::uint64_t, qt_script> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        std::unique_ptr<request_interceptor> interceptor;
        std::unordered_map<std::string, scheme::handler> schemes;

      public:
        template <event>
        void setup(impl *);

      public:
        QWebEngineScript find(const char *) const;

      public:
        static bool init_web_channel();
        static inline std::string channel_script{};

      public:
        static constexpr const auto *ready_script      = "saucer_ready";
        static constexpr const auto *creation_script   = "saucer_creation";
        static constexpr const auto *script_identifier = "//@saucer-script:{}";
    };

    struct web_class : QObject
    {
        Q_OBJECT

      private:
        webview::impl *impl;

      public:
        web_class(webview::impl *);

      public slots:
        void on_message(const QString &);
    };

    struct request_interceptor : QWebEngineUrlRequestInterceptor
    {
        Q_OBJECT

      private:
        webview::impl *impl;

      public:
        request_interceptor(webview::impl *);

      public:
        void interceptRequest(QWebEngineUrlRequestInfo &) override;
    };
} // namespace saucer
