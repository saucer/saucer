#include "webview.qt.impl.hpp"

#include <QFile>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

namespace saucer
{
    const std::string webview::impl::inject_script = []() {
        QFile web_channel_api(":/qtwebchannel/qwebchannel.js");

        if (!web_channel_api.open(QIODevice::ReadOnly))
        {
            throw std::runtime_error("Failed to open required qwebchannel.js");
        }

        auto content = web_channel_api.readAll().toStdString();
        web_channel_api.close();

        return content +
               R"js(
                window.saucer = {
                    async on_message(message)
                    {
                        (await window._saucer).on_message(message);
                    }
                };

                window._saucer = new Promise((resolve) =>
                {
                    new QWebChannel(qt.webChannelTransport, function(channel) { 
                        resolve(channel.objects.saucer); 
                    });
                });
                
                window.saucer.on_message("js_ready");
            )js";
    }();

    webview::impl::web_class::web_class(webview *parent) : QObject(parent->m_impl->web_view), m_parent(parent) {}

    void webview::impl::web_class::on_message(const QString &message)
    {
        m_parent->on_message(message.toStdString());
    }

    webview::impl::url_scheme_handler::url_scheme_handler(webview *parent)
        : QWebEngineUrlSchemeHandler(parent->m_impl->web_view), m_parent(parent)
    {
    }

    void webview::impl::url_scheme_handler::requestStarted(QWebEngineUrlRequestJob *request)
    {
        auto url = request->requestUrl().toString(QUrl::RemoveQuery).toStdString();

        if (!url.starts_with(scheme_prefix))
        {
            request->fail(QWebEngineUrlRequestJob::UrlInvalid);
            return;
        }

        url = url.substr(scheme_prefix.size());

        if (!m_parent->m_embedded_files.contains(url))
        {
            request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }

        const auto &file = m_parent->m_embedded_files.at(url);

        auto *buffer = new QBuffer;

        buffer->open(QIODevice::WriteOnly);
        buffer->write(reinterpret_cast<const char *>(file.data), static_cast<std::int64_t>(file.size));

        buffer->close();
        connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);

        request->reply(QString::fromStdString(file.mime).toUtf8(), buffer);
    }
} // namespace saucer