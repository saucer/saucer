#pragma once

#include <saucer/scheme.hpp>

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
        scheme::resolver resolver;

      public:
        handler(scheme::resolver);

      public:
        handler(handler &&) noexcept;

      public:
        void requestStarted(QWebEngineUrlRequestJob *) override;
    };
} // namespace saucer::scheme
