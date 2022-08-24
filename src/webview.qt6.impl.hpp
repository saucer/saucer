#pragma once
#include "webview.hpp"

#include <QFile>
#include <QBuffer>
#include <QObject>
#include <cassert>
#include <string_view>
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
        static const std::string inject_script;
        static inline constexpr std::string_view scheme_prefix = "saucer:/embedded/";
    };

    inline const std::string webview::impl::inject_script = []() {
        QFile web_channel_api(":/qtwebchannel/qwebchannel.js");

        if (!web_channel_api.open(QIODevice::ReadOnly))
        {
            assert((void("Failed to open required qwebchannel.js"), false)); // NOLINT
        }

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
        saucer_web_class(QObject *parent_obj, webview *parent) : QObject(parent_obj), m_parent(parent) {}

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
        saucer_url_scheme_handler(QObject *parent_obj, webview *parent) : QWebEngineUrlSchemeHandler(parent_obj), m_parent(parent) {}

      public:
        void requestStarted(QWebEngineUrlRequestJob *request) override
        {
            auto url = request->requestUrl().toString().toStdString();
            if (url.size() > scheme_prefix.size())
            {
                url = url.substr(scheme_prefix.size());
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
                buffer->write(reinterpret_cast<const char *>(file.data), static_cast<std::int64_t>(file.size));
                buffer->close();

                connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);
                request->reply(QString::fromStdString(file.mime).toUtf8(), buffer);
            }
            else
            {
                request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            }
        }
    };
} // namespace saucer