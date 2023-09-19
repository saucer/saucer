#include "webview.hpp"
#include "window.qt.impl.hpp"
#include "webview.qt.impl.hpp"

#include <fmt/core.h>
#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineScriptCollection>

namespace saucer
{
    const auto register_scheme = []
    {
        using Flags = QWebEngineUrlScheme::Flag;
        auto scheme = QWebEngineUrlScheme("saucer");

        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
        scheme.setFlags(Flags::LocalScheme | Flags::LocalAccessAllowed | Flags::SecureScheme);

        QWebEngineUrlScheme::registerScheme(scheme);
    };

    webview::webview(const options &options) : window(options), m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;
        std::call_once(flag, register_scheme);

        m_impl->web_view = new QWebEngineView(window::m_impl->window);

        auto profile_name = fmt::format("profile-{}", options.storage_path.string());
        m_impl->profile = new QWebEngineProfile(QString::fromStdString(profile_name), m_impl->web_view);

        m_impl->page = new impl::web_page(m_impl->profile, m_impl->web_view);

        m_impl->profile->setPersistentCookiesPolicy(options.persistent_cookies
                                                        ? QWebEngineProfile::AllowPersistentCookies
                                                        : QWebEngineProfile::NoPersistentCookies);

        if (!options.storage_path.empty())
        {
            m_impl->profile->setPersistentStoragePath(QString::fromStdString(options.storage_path));
        }

        m_impl->web_view->setPage(m_impl->page);

        m_impl->web_channel = new QWebChannel(m_impl->web_view);
        m_impl->web_view->page()->setWebChannel(m_impl->web_channel);

        m_impl->channel_obj = new impl::web_class(this);
        m_impl->web_channel->registerObject("saucer", m_impl->channel_obj);

        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::loadStarted,
                                  [this]() { m_impl->is_ready = false; });

        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::loadFinished,
                                  [this]() { m_impl->is_ready = true; });

        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::urlChanged,
                                  [this](const QUrl &url) { on_url_changed(url.toString().toStdString()); });

        window::m_impl->on_closed = [this]
        {
            set_dev_tools(false);
        };

        inject(impl::inject_script, load_time::creation);

        window::m_impl->window->setCentralWidget(m_impl->web_view);
        m_impl->web_view->show();
    }

    // ? The window destructor will implicitly delete the web_view
    webview::~webview() = default;

    void webview::on_url_changed(const std::string &url)
    {
        m_events.at<web_event::url_changed>().fire(url);
    }

    bool webview::dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return dev_tools(); });
        }

        return static_cast<bool>(m_impl->dev_view);
    }

    std::string webview::url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return url(); });
        }

        return m_impl->web_view->url().toString().toStdString();
    }

    bool webview::context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return context_menu(); });
        }

        return m_impl->web_view->contextMenuPolicy() == Qt::ContextMenuPolicy::DefaultContextMenu;
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, enabled] { return set_dev_tools(enabled); });
        }

        if (!enabled)
        {
            if (m_impl->dev_view)
            {
                m_impl->dev_view->deleteLater();
                m_impl->dev_view = nullptr;
            }

            return;
        }

        if (!m_impl->dev_view)
        {
            m_impl->dev_view = new QWebEngineView;
            m_impl->web_view->page()->setDevToolsPage(m_impl->dev_view->page());
        }

        m_impl->dev_view->show();
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, enabled] { return set_context_menu(enabled); });
        }

        m_impl->web_view->setContextMenuPolicy(enabled ? Qt::ContextMenuPolicy::DefaultContextMenu
                                                       : Qt::ContextMenuPolicy::NoContextMenu);
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, url] { return set_url(url); });
        }

        m_impl->web_view->setUrl(QString::fromStdString(url));
    }

    void webview::embed(embedded_files &&files)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, files = std::move(files)]() mutable
                                             { return embed(std::move(files)); });
        }

        m_embedded_files.merge(files);

        if (m_impl->scheme_handler)
        {
            return;
        }

        m_impl->scheme_handler = new impl::url_scheme_handler(this);
        m_impl->web_view->page()->profile()->installUrlSchemeHandler("saucer", m_impl->scheme_handler);
    }
    void webview::serve(const std::string &file)
    {
        set_url(fmt::format("{}{}", impl::scheme_prefix, file));
    }

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_scripts(); });
        }

        m_impl->web_view->page()->scripts().clear();
        inject(impl::inject_script, load_time::creation);
    }

    void webview::clear_embedded()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();

        if (!m_impl->scheme_handler)
        {
            return;
        }

        m_impl->web_view->page()->profile()->removeUrlSchemeHandler(m_impl->scheme_handler);
        m_impl->scheme_handler->deleteLater();
        m_impl->scheme_handler = nullptr;
    }

    void webview::clear(web_event event)
    {
        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <>
    std::uint64_t webview::on<web_event::url_changed>(events::type_t<web_event::url_changed> &&callback)
    {
        return m_events.at<web_event::url_changed>().add(std::move(callback));
    }
} // namespace saucer