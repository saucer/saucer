#pragma once
#include "webview.hpp"

#include <QFile>
#include <QBuffer>
#include <QObject>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

namespace saucer
{
    struct webview::impl
    {
        QWebEngineUrlSchemeHandler *url_scheme_handler;
        std::unique_ptr<webview> dev_view;
        QWebEngineView *web_view;
        QWebChannel *web_channel;
        QObject *web_class;

        bool is_ready{false};

        class saucer_web_class;
        class saucer_url_scheme_handler;
        static std::string inject_script;
    };

    inline std::string webview::impl::inject_script = []() {
        QFile web_channel_api(":/qtwebchannel/qwebchannel.js");
        assert((void("Failed to open required qwebchannel.js"), web_channel_api.open(QIODevice::ReadOnly)));

        auto content = web_channel_api.readAll().toStdString();
        web_channel_api.close();

        return content + R"js(
                    window.saucer = {
                        async on_message(message)
                        {
                            (await window._saucer).on_message(message);
                        }
                    };
                    
                    window._saucer = new Promise((resolve) =>
                    {
                        new QWebChannel(qt.webChannelTransport, function(channel) { resolve(channel.objects.saucer); });
                    });

                    window.saucer.on_message("js_ready");
                    )js";
    }();

    class webview::impl::saucer_web_class : public QObject
    {
        Q_OBJECT

      private:
        webview *m_parent;

      public:
        using QObject::QObject;

      public:
        void set_parent(webview *parent)
        {
            m_parent = parent;
        }

      public slots:
        void on_message(const QString &message)
        {
            m_parent->on_message(message.toStdString());
        }
    };

    class webview::impl::saucer_url_scheme_handler : public QWebEngineUrlSchemeHandler
    {
      private:
        webview *m_parent;

      public:
        using QWebEngineUrlSchemeHandler::QWebEngineUrlSchemeHandler;

      public:
        void set_parent(webview *parent)
        {
            m_parent = parent;
        }

        void requestStarted(QWebEngineUrlRequestJob *request) override
        {
            auto url = request->requestUrl().toString().toStdString();
            if (url.size() > 9)
            {
                url = url.substr(9);
            }
            else
            {
                request->fail(QWebEngineUrlRequestJob::UrlInvalid);
                return;
            }

            if (m_parent->m_embedded_files.count(url))
            {
                const auto &file = m_parent->m_embedded_files.at(url);

                auto *buffer = new QBuffer;
                buffer->open(QIODevice::WriteOnly);
                buffer->write(reinterpret_cast<const char *>(std::get<2>(file)), static_cast<std::int64_t>(std::get<1>(file)));
                buffer->close();

                connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);
                request->reply(QString::fromStdString(std::get<0>(file)).toUtf8(), buffer);
            }
            else
            {
                request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            }
        }
    };
} // namespace saucer

// #include "webview.qt5.impl.inl.moc"