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

    class stream_handler : public QWebEngineUrlSchemeHandler
    {
        scheme::stream_resolver resolver;

      public:
        stream_handler(scheme::stream_resolver);

      public:
        stream_handler(stream_handler &&) noexcept;

      public:
        void requestStarted(QWebEngineUrlRequestJob *) override;
    };

    struct stream_writer::impl
    {
        std::shared_ptr<lockpp::lock<QWebEngineUrlRequestJob *>> request;
        class stream_device *device;
        std::atomic<bool> started{false};
        std::atomic<bool> finished{false};
    };
} // namespace saucer::scheme
