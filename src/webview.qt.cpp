#include "webview.hpp"
#include "webview.qt.impl.hpp"

#include "requests.hpp"
#include "instantiate.hpp"
#include "window.qt.impl.hpp"

#include <fmt/core.h>
#include <filesystem>

#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>
#include <QWebEngineScriptCollection>

namespace saucer
{
    namespace fs = std::filesystem;

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

        auto *profile = new QWebEngineProfile("saucer");

        if (!options.storage_path.empty())
        {
            auto path = QString::fromStdString(options.storage_path.string());

            profile->setCachePath(path);
            profile->setPersistentStoragePath(path);
        }

        profile->setPersistentCookiesPolicy(options.persistent_cookies ? QWebEngineProfile::ForcePersistentCookies
                                                                       : QWebEngineProfile::NoPersistentCookies);

        profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

        m_impl->web_view    = new QWebEngineView(window::m_impl->window);
        m_impl->page        = new QWebEnginePage(profile, m_impl->web_view);
        m_impl->web_channel = new QWebChannel(m_impl->web_view);

        m_impl->web_view->setPage(m_impl->page);
        m_impl->web_view->page()->setWebChannel(m_impl->web_channel);

        m_impl->channel_obj = new impl::web_class(this);
        m_impl->web_channel->registerObject("saucer", m_impl->channel_obj);

        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::loadStarted,
                                  [this]()
                                  {
                                      m_impl->dom_loaded = false;
                                      m_events.at<web_event::load_started>().fire();
                                  });

        window::m_impl->on_closed = [this]
        {
            set_dev_tools(false);
        };

        inject(impl::ready_script, load_time::ready);
        inject(impl::inject_script, load_time::creation);

        window::m_impl->window->setCentralWidget(m_impl->web_view);
        m_impl->web_view->show();
    }

    // ? The window destructor will implicitly delete the web_view
    webview::~webview() = default;

    bool webview::on_message(const std::string &message)
    {
        if (message == "dom_loaded")
        {
            m_impl->dom_loaded = true;

            for (const auto &pending : m_impl->pending)
            {
                execute(pending);
            }

            m_impl->pending.clear();
            m_events.at<web_event::dom_ready>().fire();

            return true;
        }

        auto request = requests::parse(message);

        if (!request)
        {
            return false;
        }

        if (std::holds_alternative<requests::resize>(request.value()))
        {
            const auto data = std::get<requests::resize>(request.value());
            start_resize(static_cast<window_edge>(data.edge));

            return true;
        }

        if (std::holds_alternative<requests::drag>(request.value()))
        {
            start_drag();
            return true;
        }

        return false;
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

        if (!m_impl->dev_view && !enabled)
        {
            return;
        }

        if (!enabled)
        {
            m_impl->web_view->page()->setDevToolsPage(nullptr);

            m_impl->dev_view->close();
            m_impl->dev_view->deleteLater();

            m_impl->dev_view = nullptr;

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

    void webview::set_file(const std::string &file)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, file] { return set_file(file); });
        }

        auto path = fs::canonical(file).string();
        m_impl->web_view->setUrl(QUrl::fromLocalFile(QString::fromStdString(path)));
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

        inject(impl::ready_script, load_time::ready);
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

    void webview::execute(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, java_script] { execute(java_script); });
        }

        if (!m_impl->dom_loaded)
        {
            m_impl->pending.emplace_back(java_script);
            return;
        }

        m_impl->web_view->page()->runJavaScript(QString::fromStdString(java_script));
    }

    void webview::clear(web_event event)
    {
        switch (event)
        {
        case web_event::load_finished:
            m_impl->web_view->disconnect(m_impl->load_finished);
            break;
        case web_event::url_changed:
            m_impl->web_view->disconnect(m_impl->url_changed);
            break;
        default:
            break;
        };

        m_events.clear(event);
    }

    void webview::remove(web_event event, std::uint64_t id)
    {
        m_events.remove(event, id);
    }

    template <web_event Event>
    void webview::once(events::type_t<Event> &&callback)
    {
        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type_t<Event> &&callback)
    {
        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    INSTANTIATE_EVENTS(webview, 4, web_event)
} // namespace saucer
