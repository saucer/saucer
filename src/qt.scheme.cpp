#include "qt.scheme.impl.hpp"

#include "qt.url.impl.hpp"

#include <ranges>

#include <QMap>
#include <QBuffer>
#include <QIODevice>

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    url request::url() const
    {
        const auto request = m_impl->request->write();
        return url::impl{request.value()->requestUrl()};
    }

    std::string request::method() const
    {
        const auto request = m_impl->request->write();
        return request.value()->requestMethod().toStdString();
    }

    stash<> request::content() const
    {
        const auto *data = reinterpret_cast<const std::uint8_t *>(m_impl->body.data());
        return stash<>::view({data, data + m_impl->body.size()});
    }

    std::map<std::string, std::string> request::headers() const
    {
        const auto request = m_impl->request->write();
        const auto headers = request.value()->requestHeaders();

        auto transform = [&headers](auto &item)
        {
            return std::make_pair(item.toStdString(), headers[item].toStdString());
        };

        return headers                            //
               | std::views::transform(transform) //
               | std::ranges::to<std::map<std::string, std::string>>();
    }

    handler::handler(scheme::resolver resolver) : resolver(std::move(resolver)) {}

    handler::handler(handler &&other) noexcept : resolver(std::move(other.resolver)) {}

    void handler::requestStarted(QWebEngineUrlRequestJob *raw)
    {
        if (!resolver)
        {
            return;
        }

        auto request = std::make_shared<lockpp::lock<QWebEngineUrlRequestJob *>>(raw);
        auto content = QByteArray{};

#ifdef SAUCER_QT6
        auto *const body = raw->requestBody();

        if (body && body->open(QIODevice::OpenModeFlag::ReadOnly))
        {
            content = body->readAll();
        }
#endif

        auto resolve = [request](const scheme::response &response)
        {
            const auto req = request->write();

            if (!req.value())
            {
                return;
            }

#ifdef SAUCER_QT6
            auto to_array = [](auto &item)
            {
                return std::make_pair(QByteArray::fromStdString(item.first), QByteArray::fromStdString(item.second));
            };

            const auto headers   = std::views::transform(response.headers, to_array);
            const auto converted = QMultiMap<QByteArray, QByteArray>{{headers.begin(), headers.end()}};

            req.value()->setAdditionalResponseHeaders(converted);
#endif

            const auto data = response.data;
            auto *buffer    = new QBuffer{};

            buffer->open(QIODevice::WriteOnly);
            buffer->write(reinterpret_cast<const char *>(data.data()), static_cast<std::int64_t>(data.size()));
            buffer->close();

            connect(req.value(), &QObject::destroyed, buffer, &QObject::deleteLater);
            req.value()->reply(QString::fromStdString(response.mime).toUtf8(), buffer);
        };

        auto reject = [request](const scheme::error &error)
        {
            const auto req = request->write();

            if (!req.value())
            {
                return;
            }

            QWebEngineUrlRequestJob::Error err{};

            switch (error)
            {
                using enum scheme::error;
                using enum QWebEngineUrlRequestJob::Error;

            case not_found:
                err = UrlNotFound;
                break;
            case invalid:
                err = UrlInvalid;
                break;
            case denied:
                err = RequestDenied;
                break;
            case failed:
                err = RequestFailed;
                break;
            }

            req.value()->fail(err);
        };

        auto executor = scheme::executor{std::move(resolve), std::move(reject)};
        auto req      = scheme::request{{.request = request, .body = std::move(content)}};

        connect(raw, &QObject::destroyed, [request]() { request->assign(nullptr); });

        return std::invoke(resolver, std::move(req), std::move(executor));
    }
} // namespace saucer::scheme
