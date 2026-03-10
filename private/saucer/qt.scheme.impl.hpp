#pragma once

#include <saucer/scheme.hpp>
#include <saucer/stash.impl.hpp>

#include <QBuffer>
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

    struct stash_stream : stash::impl
    {
        struct native;

      public:
        std::shared_ptr<native> platform;

      public:
        stash_stream();

      public:
        [[nodiscard]] stash::span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };

    class stream_device : public QIODevice
    {
        QByteArray buffer;

      protected:
        qint64 readData(char *, qint64) override;
        qint64 writeData(const char *, qint64) override;

      protected:
        [[nodiscard]] bool isSequential() const override;
        [[nodiscard]] qint64 bytesAvailable() const override;

      public:
        void append(stash::span);
    };

    struct stash_stream::native
    {
        std::unique_ptr<stream_device> device;
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
