#include "qt.scheme.impl.hpp"

#include <lockpp/lock.hpp>

#include <ranges>
#include <QtGlobal>

#include <QMap>
#include <QBuffer>
#include <QIODevice>

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    request::request(request &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    request::~request() = default;

    std::string request::url() const
    {
        const auto request = m_impl->request->write();

        if (!request.value())
        {
            return {};
        }

        return request.value()->requestUrl().toString().toStdString();
    }

    std::string request::method() const
    {
        const auto request = m_impl->request->write();

        if (!request.value())
        {
            return {};
        }

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

        if (!request.value())
        {
            return {};
        }

        const auto headers = request.value()->requestHeaders();

        auto transform = [&headers](auto &item)
        {
            return std::make_pair(item.toStdString(), headers[item].toStdString());
        };

        return headers                            //
               | std::views::transform(transform) //
               | std::ranges::to<std::map<std::string, std::string>>();
    }

    handler::handler(application *app, launch policy, scheme::resolver resolver)
        : app(app), policy(policy), resolver(std::move(resolver))
    {
    }

    handler::handler(handler &&other) noexcept
        : app(std::exchange(other.app, nullptr)), policy(other.policy), resolver(std::move(other.resolver))
    {
    }

    void handler::requestStarted(QWebEngineUrlRequestJob *raw)
    {
        if (!resolver)
        {
            return;
        }

        auto request = std::make_shared<lockpp::lock<QWebEngineUrlRequestJob *>>(raw);
        auto content = QByteArray{};

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        auto *const body = raw->requestBody();

        if (raw->requestBody())
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
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

            const auto offset = std::to_underlying(error) + 1;
            req.value()->fail(static_cast<QWebEngineUrlRequestJob::Error>(offset));
        };

        auto executor = scheme::executor{std::move(resolve), std::move(reject)};
        auto req      = scheme::request{{request, std::move(content)}};

        connect(raw, &QObject::destroyed, [request]() { request->assign(nullptr); });

        if (policy != launch::async)
        {
            return std::invoke(resolver, std::move(req), std::move(executor));
        }

        app->pool().emplace([resolver = resolver, executor = std::move(executor), req = std::move(req)]() mutable
                            { std::invoke(resolver, std::move(req), std::move(executor)); });
    }
} // namespace saucer::scheme
