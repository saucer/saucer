#include "qt.webview.impl.hpp"

#include "scripts.hpp"
#include "request.hpp"

#include "qt.uri.impl.hpp"
#include "qt.icon.impl.hpp"

#include "qt.navigation.impl.hpp"
#include "qt.permission.impl.hpp"

#include <format>
#include <cassert>

#include <QFile>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

namespace saucer
{
    std::string webview::impl::inject_script()
    {
        static constexpr auto internal = R"js(
            channel: new Promise((resolve) =>
            {
                new QWebChannel(qt.webChannelTransport, function(channel) {
                    resolve(channel.objects.saucer);
                });
            }),
            message: async (message) =>
            {
                (await window.saucer.internal.channel).on_message(message);
            }
        )js";

        static const auto web_channel = []
        {
            QFile qwebchannel{":/qtwebchannel/qwebchannel.js"};

            if (!qwebchannel.open(QIODevice::ReadOnly))
            {
                assert(false && "Failed to open web-channel");
            }

            return qwebchannel.readAll().toStdString();
        }();

        static const auto script = web_channel + std::format(scripts::webview_script, //
                                                             internal,                //
                                                             request::stubs());

        return script;
    }

    constinit std::string_view webview::impl::ready_script = "window.saucer.internal.message('dom_loaded')";

    webview::impl::web_class::web_class(webview *parent) : m_parent(parent) {}

    void webview::impl::web_class::on_message(const QString &raw)
    {
        auto message = raw.toStdString();
        auto &self   = *m_parent;

        if (message == "dom_loaded")
        {
            self.m_impl->dom_loaded = true;

            for (const auto &pending : self.m_impl->pending)
            {
                self.execute(pending);
            }

            self.m_impl->pending.clear();
            self.m_events.get<web_event::dom_ready>().fire();

            return;
        }

        self.on_message(message);
    }

    webview::impl::request_interceptor::request_interceptor(webview *parent) : m_parent(parent) {}

    void webview::impl::request_interceptor::interceptRequest(QWebEngineUrlRequestInfo &request)
    {
        m_parent->m_events.get<saucer::web_event::request>().fire(uri::impl{request.requestUrl()});
    }

    template <>
    void webview::impl::setup<web_event::permission>([[maybe_unused]] webview *self)
    {
#ifdef SAUCER_QT6
        auto &event = self->m_events.get<web_event::permission>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](QWebEnginePermission req)
        {
            if (req.permissionType() == QWebEnginePermission::PermissionType::Unsupported)
            {
                return;
            }

            auto request = permission::request{{
                .request = std::move(req),
            }};

            self->m_events.get<web_event::permission>().fire(request);
        };

        const auto id = web_page->connect(web_page.get(), &QWebEnginePage::permissionRequested, handler);
        event.on_clear([this, id] { web_page->disconnect(id); });
#endif
    }

    template <>
    void webview::impl::setup<web_event::dom_ready>(webview *)
    {
    }

    template <>
    void webview::impl::setup<web_event::navigated>([[maybe_unused]] webview *self)
    {
        auto &event = self->m_events.get<web_event::navigated>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const QUrl &url)
        {
            if (url.isEmpty())
            {
                return;
            }

            self->m_events.get<web_event::navigated>().fire(uri::impl{url});
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::urlChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void webview::impl::setup<web_event::navigate>([[maybe_unused]] webview *self)
    {
#ifdef SAUCER_QT6
        auto &event = self->m_events.get<web_event::navigate>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self]<typename T>(T &req)
        {
            auto request = navigation{{&req}};

            if (!self->m_events.get<web_event::navigate>().fire(request).find(policy::block))
            {
                return;
            }

            if constexpr (std::is_same_v<T, QWebEngineNavigationRequest>)
            {
                req.reject();
            }
        };

        const auto new_id = web_page->connect(web_page.get(), &QWebEnginePage::newWindowRequested, handler);
        const auto nav_id = web_page->connect(web_page.get(), &QWebEnginePage::navigationRequested, handler);

        event.on_clear(
            [this, new_id, nav_id]
            {
                web_page->disconnect(new_id);
                web_page->disconnect(nav_id);
            });
#endif
    }

    template <>
    void webview::impl::setup<web_event::request>(webview *self)
    {
        auto &event = self->m_events.get<web_event::request>();

        if (!event.empty())
        {
            return;
        }

        interceptor = std::make_unique<impl::request_interceptor>(self);
        profile->setUrlRequestInterceptor(interceptor.get());

        event.on_clear([this] { interceptor.reset(); });
    }

    template <>
    void webview::impl::setup<web_event::favicon>(webview *self)
    {
        auto &event = self->m_events.get<web_event::favicon>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const auto &favicon)
        {
            self->m_events.get<web_event::favicon>().fire(icon{{favicon}});
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::iconChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void webview::impl::setup<web_event::title>(webview *self)
    {
        auto &event = self->m_events.get<web_event::title>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const auto &title)
        {
            self->m_events.get<web_event::title>().fire(title.toStdString());
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::titleChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void webview::impl::setup<web_event::load>(webview *self)
    {
        auto &event = self->m_events.get<web_event::load>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](auto...)
        {
            self->m_events.get<web_event::load>().fire(state::finished);
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::loadFinished, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }
} // namespace saucer
