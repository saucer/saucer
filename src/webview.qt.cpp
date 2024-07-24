#include "webview.qt.impl.hpp"

#include "instantiate.hpp"
#include "window.qt.impl.hpp"

#include "scheme.hpp"
#include "requests.hpp"

#include <fmt/core.h>
#include <filesystem>

#include <QWebEngineScriptCollection>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>

namespace saucer
{
    namespace fs = std::filesystem;

    webview::webview(const options &options) : window(options), m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;
        std::call_once(flag, []() { register_scheme("saucer"); });

        m_impl->profile = std::make_unique<QWebEngineProfile>("saucer");

        if (!options.storage_path.empty())
        {
            const auto path = QString::fromStdString(options.storage_path.string());

            m_impl->profile->setCachePath(path);
            m_impl->profile->setPersistentStoragePath(path);
        }

        m_impl->profile->setPersistentCookiesPolicy(options.persistent_cookies
                                                        ? QWebEngineProfile::ForcePersistentCookies
                                                        : QWebEngineProfile::NoPersistentCookies);

        m_impl->profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

        m_impl->web_view    = std::make_unique<QWebEngineView>();
        m_impl->web_page    = std::make_unique<QWebEnginePage>(m_impl->profile.get());
        m_impl->channel     = std::make_unique<QWebChannel>();
        m_impl->channel_obj = std::make_unique<impl::web_class>(this);

        m_impl->web_view->setPage(m_impl->web_page.get());
        m_impl->web_page->setWebChannel(m_impl->channel.get());
        m_impl->channel->registerObject("saucer", m_impl->channel_obj.get());

        m_impl->web_view->connect(m_impl->web_view.get(), &QWebEngineView::loadStarted,
                                  [this]()
                                  {
                                      m_impl->dom_loaded = false;
                                      m_events.at<web_event::load_started>().fire();
                                  });

        window::m_impl->on_closed = [this]
        {
            set_dev_tools(false);
        };

        inject(impl::inject_script(), load_time::creation);
        inject(std::string{impl::ready_script}, load_time::ready);

        window::m_impl->window->setCentralWidget(m_impl->web_view.get());
        m_impl->web_view->show();
    }

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

        return static_cast<bool>(m_impl->dev_page);
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

        if (!m_impl->dev_page && !enabled)
        {
            return;
        }

        if (!enabled)
        {
            m_impl->web_page->setDevToolsPage(nullptr);
            m_impl->dev_page.reset();

            return;
        }

        if (!m_impl->dev_page)
        {
            m_impl->dev_page = std::make_unique<QWebEngineView>();
            m_impl->web_page->setDevToolsPage(m_impl->dev_page->page());
        }

        m_impl->dev_page->show();
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

    void webview::clear_scripts()
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return clear_scripts(); });
        }

        m_impl->web_view->page()->scripts().clear();

        inject(std::string{impl::ready_script}, load_time::ready);
        inject(impl::inject_script(), load_time::creation);
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

    void webview::handle_scheme(const std::string &name, scheme_handler handler)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, name, handler = std::move(handler)]
                                             { return handle_scheme(name, handler); });
        }

        if (m_impl->schemes.contains(name))
        {
            return;
        }

        auto &scheme_handler = m_impl->schemes.emplace(name, std::move(handler)).first->second;
        m_impl->web_view->page()->profile()->installUrlSchemeHandler(QByteArray::fromStdString(name), &scheme_handler);
    }

    void webview::remove_scheme(const std::string &name)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, name] { return remove_scheme(name); });
        }

        auto it = m_impl->schemes.find(name);

        if (it == m_impl->schemes.end())
        {
            return;
        }

        m_impl->web_view->page()->profile()->removeUrlSchemeHandler(&it->second);
        m_impl->schemes.erase(it);
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
    void webview::once(events::type_t<Event> callback)
    {
        m_impl->setup<Event>(this);
        m_events.at<Event>().once(std::move(callback));
    }

    template <web_event Event>
    std::uint64_t webview::on(events::type_t<Event> callback)
    {
        m_impl->setup<Event>(this);
        return m_events.at<Event>().add(std::move(callback));
    }

    INSTANTIATE_EVENTS(webview, 4, web_event)

    void webview::register_scheme(const std::string &name)
    {
        auto scheme = QWebEngineUrlScheme{name.c_str()};
        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);

        using Flags = QWebEngineUrlScheme::Flag;
        scheme.setFlags(Flags::LocalScheme | Flags::LocalAccessAllowed | Flags::SecureScheme);

        QWebEngineUrlScheme::registerScheme(scheme);
    }
} // namespace saucer
