#include "qt.scheme.impl.hpp"

#include <ranges>
#include <QtGlobal>

#include <QMap>
#include <QBuffer>
#include <QIODevice>

namespace saucer::scheme
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

    stash<> request::content() const
    {
        const auto *data = reinterpret_cast<const std::uint8_t *>(m_impl->body.data());
        return stash<>::view({data, data + m_impl->body.size()});
    }

    std::map<std::string, std::string> request::headers() const
    {
        const auto headers = m_impl->request->requestHeaders();

        auto transform = [&headers](auto &item)
        {
            return std::make_pair(item.toStdString(), headers[item].toStdString());
        };

        return headers                            //
               | std::views::transform(transform) //
               | std::ranges::to<std::map<std::string, std::string>>();
    }

    void url_scheme_handler::requestStarted(QWebEngineUrlRequestJob *request)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        auto *body = request->requestBody();
        auto req   = scheme::request{{request, body->isReadable() ? body->readAll() : QByteArray{}}};
#else
        auto req = scheme::request{{request}};
#endif

        if (!m_callback)
        {
            return;
        }

        auto result = std::invoke(m_callback, req);

        if (!result.has_value())
        {
            const auto offset = std::to_underlying(result.error()) + 1;
            request->fail(static_cast<QWebEngineUrlRequestJob::Error>(offset));

            return;
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        auto to_array = [](auto &item)
        {
            return std::make_pair(QByteArray::fromStdString(item.first), QByteArray::fromStdString(item.second));
        };

        const auto headers   = std::views::transform(result->headers, to_array);
        const auto converted = QMultiMap<QByteArray, QByteArray>{{headers.begin(), headers.end()}};

        request->setAdditionalResponseHeaders(converted);
#endif

        const auto data = result->data;
        auto *buffer    = new QBuffer{};

        buffer->open(QIODevice::WriteOnly);
        buffer->write(reinterpret_cast<const char *>(data.data()), static_cast<std::int64_t>(data.size()));
        buffer->close();

        connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);
        request->reply(QString::fromStdString(result->mime).toUtf8(), buffer);
    }
} // namespace saucer::scheme
