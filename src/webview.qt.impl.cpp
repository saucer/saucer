#include "webview.qt.impl.hpp"

#include "scripts.hpp"
#include "icon.qt.impl.hpp"

#include <optional>
#include <fmt/core.h>

#include <QFile>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

namespace saucer
{
    const std::string &webview::impl::inject_script()
    {
        static std::optional<std::string> instance;

        if (instance)
        {
            return instance.value();
        }

        QFile qwebchannel{":/qtwebchannel/qwebchannel.js"};

        if (!qwebchannel.open(QIODevice::ReadOnly))
        {
            throw std::runtime_error{"Failed to open required qwebchannel.js"};
        }

        auto content = qwebchannel.readAll().toStdString();
        qwebchannel.close();

        instance.emplace(content + fmt::format(scripts::webview_script, fmt::arg("internal", R"js(
        channel: new Promise((resolve) =>
        {
            new QWebChannel(qt.webChannelTransport, function(channel) {
                resolve(channel.objects.saucer);
            });
        }),
        send_message: async (message) =>
        {
            (await window.saucer.internal.channel).on_message(message);
        }
        )js")));

        return instance.value();
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.send_message('dom_loaded')";

    webview::impl::web_class::web_class(webview *parent) : m_parent(parent) {}

    void webview::impl::web_class::on_message(const QString &message)
    {
        m_parent->on_message(message.toStdString());
    }

    template <>
    void webview::impl::setup<web_event::title_changed>(webview *self)
    {
        if (title_changed)
        {
            return;
        }

        auto handler = [self](const auto &title)
        {
            self->m_events.at<web_event::title_changed>().fire(title.toStdString());
        };

        title_changed = web_view->connect(web_view.get(), &QWebEngineView::titleChanged, handler);
    }

    template <>
    void webview::impl::setup<web_event::load_finished>(webview *self)
    {
        if (load_finished)
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.at<web_event::load_finished>().fire();
        };

        load_finished = web_view->connect(web_view.get(), &QWebEngineView::loadFinished, handler);
    }

    template <>
    void webview::impl::setup<web_event::icon_changed>(webview *self)
    {
        if (icon_changed)
        {
            return;
        }

        auto handler = [self](const auto &favicon)
        {
            self->m_events.at<web_event::icon_changed>().fire(icon{{favicon}});
        };

        icon_changed = web_view->connect(web_view.get(), &QWebEngineView::iconChanged, handler);
    }

    template <>
    void webview::impl::setup<web_event::load_started>(webview *)
    {
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
            if (url.isEmpty())
            {
                return;
            }

            self->m_events.at<web_event::url_changed>().fire(url.toString().toStdString());
        };

        url_changed = web_view->connect(web_view.get(), &QWebEngineView::urlChanged, handler);
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }
} // namespace saucer
