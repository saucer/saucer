#pragma once

#include "scheme.hpp"

#include "webview.hpp"

#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

#include <lockpp/lock.hpp>

namespace saucer::scheme
{
    struct request::impl
    {
        std::shared_ptr<lockpp::lock<QWebEngineUrlRequestJob *>> request;
        QByteArray body;
    };

    class handler : public QWebEngineUrlSchemeHandler
    {
        application *app;
        scheme::resolver resolver;

      public:
        handler(application *, scheme::resolver);

      public:
        handler(handler &&) noexcept;

      public:
        void requestStarted(QWebEngineUrlRequestJob *) override;
    };
} // namespace saucer::scheme
