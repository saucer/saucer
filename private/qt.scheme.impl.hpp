#pragma once

#include "scheme.hpp"

#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

namespace saucer
{
    struct request::impl
    {
        QWebEngineUrlRequestJob *request;
        QByteArray body;
    };

    class url_scheme_handler : public QWebEngineUrlSchemeHandler
    {
        scheme_handler m_callback;

      public:
        url_scheme_handler(scheme_handler callback) : m_callback(std::move(callback)) {}

      public:
        void requestStarted(QWebEngineUrlRequestJob *) override;
    };
} // namespace saucer
