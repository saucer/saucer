#include "webview.qt.impl.hpp"

#include <QFile>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

namespace saucer
{
    constinit std::string_view webview::impl::ready_script = "window.saucer.on_message('dom_loaded')";

    const std::string &webview::impl::inject_script()
    {
        static std::unique_ptr<std::string> instance;

        if (instance)
        {
            return *instance;
        }

        QFile qwebchannel{":/qtwebchannel/qwebchannel.js"};

        if (!qwebchannel.open(QIODevice::ReadOnly))
        {
            throw std::runtime_error("Failed to open required qwebchannel.js");
        }

        auto content = qwebchannel.readAll().toStdString();
        qwebchannel.close();

        instance = std::make_unique<std::string>(content +
                                                 R"js(
                window.saucer = 
                {
                    window_edge:
                    {
                        top:    1,
                        bottom: 2,
                        left:   4,
                        right:  8,
                    },
                    on_message: async (message) =>
                    {
                        (await window._saucer).on_message(message);
                    },
                    start_drag: async () =>
                    {
                        await window.saucer.on_message(JSON.stringify({
                            ["saucer:drag"]: true
                        }));
                    },
                    start_resize: async (edge) =>
                    {
                        await window.saucer.on_message(JSON.stringify({
                            ["saucer:resize"]: true,
                            edge,
                        }));
                    }
                };

                window._saucer = new Promise((resolve) =>
                {
                    new QWebChannel(qt.webChannelTransport, function(channel) { 
                        resolve(channel.objects.saucer); 
                    });
                });
            )js");

        return *instance;
    }

    webview::impl::web_class::web_class(webview *parent) : m_parent(parent) {}

    void webview::impl::web_class::on_message(const QString &message)
    {
        m_parent->on_message(message.toStdString());
    }

    template <>
    void webview::impl::setup<web_event::load_finished>(webview *self)
    {
        if (load_finished)
        {
            return;
        }

        auto handler = [self](bool success)
        {
            if (!success)
            {
                return;
            }

            self->m_events.at<web_event::load_finished>().fire();
        };

        load_finished = web_view->connect(web_view.get(), &QWebEngineView::loadFinished, handler);
    }

    template <>
    void webview::impl::setup<web_event::url_changed>(webview *self)
    {
        if (url_changed)
        {
            return;
        }

        auto handler = [self](const QUrl &url)
        {
            self->m_events.at<web_event::url_changed>().fire(url.toString().toStdString());
        };

        url_changed = web_view->connect(web_view.get(), &QWebEngineView::urlChanged, handler);
    }

    template <>
    void webview::impl::setup<web_event::load_started>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }
} // namespace saucer
