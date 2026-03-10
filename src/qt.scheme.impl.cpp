#include "qt.scheme.impl.hpp"

#include "modules/stable/qt.hpp"

#include <QMap>
#include <QBuffer>
#include <QIODevice>

namespace saucer::scheme
{
    stash_stream::stash_stream() : platform(std::make_shared<native>(std::make_unique<stream_device>())) {}

    std::size_t stash_stream::type() const
    {
        return id_of<stash_stream>();
    }

    std::unique_ptr<stash::impl> stash_stream::clone() const
    {
        return std::make_unique<stash_stream>(*this);
    }

    stash::span stash_stream::data() const
    {
        return {};
    }

    qint64 stream_device::readData(char *data, qint64 requested)
    {
        const auto size = std::min(requested, buffer.size());
        memcpy(data, buffer.data(), size);
        buffer.remove(0, size);
        return size;
    }

    qint64 stream_device::writeData(const char *, qint64)
    {
        return -1;
    }

    bool stream_device::isSequential() const
    {
        return true;
    }

    qint64 stream_device::bytesAvailable() const
    {
        return buffer.size() + QIODevice::bytesAvailable();
    }

    void stream_device::append(stash::span data)
    {
        buffer.append(reinterpret_cast<const char *>(data.data()), static_cast<qsizetype>(data.size()));
        emit readyRead();
    }

    std::pair<QIODevice *, std::function<void()>> device_of(const stash &stash)
    {
        if (auto *const device = stash.native().device; device)
        {
            return {device, [platform = static_cast<stash_stream *>(stash.native<false>())->platform] {}};
        }

        const auto data    = stash.data();
        auto *const buffer = new QBuffer{};

        buffer->open(QIODevice::WriteOnly);
        buffer->write(reinterpret_cast<const char *>(data.data()), static_cast<std::int64_t>(data.size()));
        buffer->close();

        return {buffer, std::bind_front(&QObject::deleteLater, buffer)};
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

        auto *const body = raw->requestBody();

        if (body && body->open(QIODevice::OpenModeFlag::ReadOnly))
        {
            content = body->readAll();
        }

        auto resolve = [request](const scheme::response &response)
        {
            const auto req = request->write();

            if (!req.value())
            {
                return;
            }

            auto to_array = [](auto &item)
            {
                return std::make_pair(QByteArray::fromStdString(item.first), QByteArray::fromStdString(item.second));
            };

            const auto headers   = std::views::transform(response.headers, to_array);
            const auto converted = QMultiMap<QByteArray, QByteArray>{{headers.begin(), headers.end()}};

            req.value()->setAdditionalResponseHeaders(converted);

            const auto stash             = response.data;
            const auto [device, release] = device_of(stash);

            connect(req.value(), &QObject::destroyed, device, release);
            req.value()->reply(QString::fromStdString(response.mime).toUtf8(), device);
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

        return resolver(std::move(req), std::move(executor));
    }
} // namespace saucer::scheme
