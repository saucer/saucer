#include "qt.webview.impl.hpp"

#include "qt.url.impl.hpp"
#include "qt.icon.impl.hpp"

#include "qt.navigation.impl.hpp"
#include "qt.permission.impl.hpp"

#include <QFile>
#include <QBuffer>

#include <QWebEngineUrlRequestJob>
#include <QWebEngineScriptCollection>

namespace saucer
{
    using native = webview::impl::native;
    using event  = webview::event;

    web_class::web_class(webview::impl *impl) : impl(impl) {}

    void web_class::on_message(const QString &raw)
    {
        auto message = raw.toStdString();

        if (message == "dom_loaded")
        {
            return impl->events.get<event::dom_ready>().fire();
        }

        impl->events.get<event::message>().fire(message).find(status::handled);
    }

    request_interceptor::request_interceptor(webview::impl *impl) : impl(impl) {}

    void request_interceptor::interceptRequest(QWebEngineUrlRequestInfo &request)
    {
        impl->events.get<event::request>().fire(url::impl{request.requestUrl()});
    }

    template <>
    void native::setup<event::permission>([[maybe_unused]] impl *self)
    {
        using permission::request;

        auto &event = self->events.get<event::permission>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](QWebEnginePermission raw)
        {
            if (!raw.isValid())
            {
                return;
            }

            auto req = std::make_shared<request>(request::impl{
                .request = std::move(raw),
                .type    = raw.permissionType(),
                .origin  = raw.origin(),
            });

            self->events.get<event::permission>().fire(req).find(status::handled);
        };

        const auto id = web_page->connect(web_page.get(), &QWebEnginePage::permissionRequested, handler);
        event.on_clear([this, id] { web_page->disconnect(id); });
    }

    template <>
    void native::setup<event::fullscreen>(impl *)
    {
    }

    template <>
    void native::setup<event::dom_ready>(impl *)
    {
    }

    template <>
    void native::setup<event::navigated>([[maybe_unused]] impl *self)
    {
        auto &event = self->events.get<event::navigated>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const QUrl &url)
        {
            self->events.get<event::navigated>().fire(url::impl{url});
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::urlChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void native::setup<event::navigate>([[maybe_unused]] impl *self)
    {
        auto &event = self->events.get<event::navigate>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self]<typename T>(T &req)
        {
            auto request = navigation{navigation::impl{&req}};

            if (!self->events.get<event::navigate>().fire(request).find(policy::block))
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
    }

    template <>
    void native::setup<event::message>(impl *)
    {
    }

    template <>
    void native::setup<event::request>(impl *self)
    {
        auto &event = self->events.get<event::request>();

        if (!event.empty())
        {
            return;
        }

        interceptor = std::make_unique<request_interceptor>(self);
        profile->setUrlRequestInterceptor(interceptor.get());

        event.on_clear([this] { interceptor.reset(); });
    }

    template <>
    void native::setup<event::favicon>(impl *self)
    {
        auto &event = self->events.get<event::favicon>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const auto &favicon)
        {
            self->events.get<event::favicon>().fire(icon{icon::impl{favicon}});
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::iconChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void native::setup<event::title>(impl *self)
    {
        auto &event = self->events.get<event::title>();

        if (!event.empty())
        {
            return;
        }

        auto handler = [self](const auto &title)
        {
            self->events.get<event::title>().fire(title.toStdString());
        };

        const auto id = web_view->connect(web_view.get(), &QWebEngineView::titleChanged, handler);
        event.on_clear([this, id] { web_view->disconnect(id); });
    }

    template <>
    void native::setup<event::load>(impl *self)
    {
        auto &event = self->events.get<event::load>();

        if (!event.empty())
        {
            return;
        }

        auto start = [self](auto...)
        {
            self->events.get<event::load>().fire(state::started);
        };

        auto finish = [self](auto...)
        {
            self->events.get<event::load>().fire(state::finished);
        };

        const auto start_id  = web_view->connect(web_view.get(), &QWebEngineView::loadStarted, start);
        const auto finish_id = web_view->connect(web_view.get(), &QWebEngineView::loadFinished, finish);

        event.on_clear(
            [this, start_id, finish_id]
            {
                web_view->disconnect(start_id);
                web_view->disconnect(finish_id);
            });
    }

    QWebEngineScript native::find(const char *name) const
    {
        return web_page->scripts().find(name).at(0);
    }

    bool native::init_web_channel()
    {
        if (!channel_script.empty())
        {
            return true;
        }

        auto file = QFile{":/qtwebchannel/qwebchannel.js"};

        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        channel_script = file.readAll().toStdString();

        return true;
    }
} // namespace saucer
