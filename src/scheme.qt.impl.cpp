#include "scheme.qt.impl.hpp"

#include <ranges>

#include <QMap>
#include <QBuffer>
#include <QtGlobal>
#include <QIODevice>

namespace saucer
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::~request() = default;

    std::string request::url() const
    {
        return m_impl->request->requestUrl().toString().toStdString();
    }

    std::string request::method() const
    {
        return m_impl->request->requestMethod().toStdString();
    }

    std::span<std::uint8_t> request::content() const
    {
        auto *data = reinterpret_cast<std::uint8_t *>(m_impl->body.data());
        return {data, data + m_impl->body.size()};
    }

    std::map<std::string, std::string> request::headers() const
    {
        auto headers = m_impl->request->requestHeaders();
        auto rtn     = std::views::transform(headers, [&headers](auto &item)
                                             { return std::make_pair(item.toStdString(), headers[item].toStdString()); });

        return {rtn.begin(), rtn.end()};
    }

    void url_scheme_handler::requestStarted(QWebEngineUrlRequestJob *request)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        auto req = saucer::request{{request, request->requestBody()->readAll()}};
#else
        auto req = saucer::request{{request}};
#endif

        if (!m_callback)
        {
            return;
        }

        auto result = std::invoke(m_callback, req);

        if (!result.has_value())
        {
            switch (result.error())
            {
            case request_error::aborted:
                return request->fail(QWebEngineUrlRequestJob::RequestAborted);
            case request_error::bad_url:
                return request->fail(QWebEngineUrlRequestJob::UrlInvalid);
            case request_error::denied:
                return request->fail(QWebEngineUrlRequestJob::RequestDenied);
            case request_error::not_found:
                return request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            default:
            case request_error::failed:
                return request->fail(QWebEngineUrlRequestJob::RequestFailed);
            }
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        auto to_array = [](auto &item)
        {
            return std::make_pair(QByteArray::fromStdString(item.first), QByteArray::fromStdString(item.second));
        };

        auto headers = std::views::transform(result->headers, to_array);
        request->setAdditionalResponseHeaders(QMultiMap<QByteArray, QByteArray>{{headers.begin(), headers.end()}});
#endif

        auto data    = result->data;
        auto *buffer = new QBuffer{};

        buffer->open(QIODevice::WriteOnly);
        buffer->write(reinterpret_cast<const char *>(data.data()), static_cast<std::int64_t>(data.size()));
        buffer->close();

        connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);
        request->reply(QString::fromStdString(result->mime).toUtf8(), buffer);
    }
} // namespace saucer
