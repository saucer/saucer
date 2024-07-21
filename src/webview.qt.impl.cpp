#include "webview.qt.impl.hpp"

#include <QFile>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

namespace saucer
{
    const std::string webview::impl::ready_script = "window.saucer.on_message('dom_loaded')";

    const std::string webview::impl::inject_script = []()
    {
        QFile web_channel_api(":/qtwebchannel/qwebchannel.js");

        if (!web_channel_api.open(QIODevice::ReadOnly))
        {
            throw std::runtime_error("Failed to open required qwebchannel.js");
        }

        auto content = web_channel_api.readAll().toStdString();
        web_channel_api.close();

        return content +
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
            )js";
    }();

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
