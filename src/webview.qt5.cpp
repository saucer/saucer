#include "webview.hpp"
#include "window.qt5.impl.hpp"
#include "webview.qt5.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineScriptCollection>

namespace saucer
{
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        static std::once_flag flag;
        std::call_once(flag, [] { QWebEngineUrlScheme::registerScheme(QWebEngineUrlScheme("saucer")); });

        m_impl->web_view = new QWebEngineView(window::m_impl->window);
        window::m_impl->window->setCentralWidget(m_impl->web_view);

        m_impl->web_class = new impl::saucer_web_class(m_impl->web_view);
        dynamic_cast<impl::saucer_web_class *>(m_impl->web_class)->set_parent(this);

        m_impl->web_channel = new QWebChannel(m_impl->web_view);
        m_impl->web_view->page()->setWebChannel(m_impl->web_channel);
        m_impl->web_channel->registerObject("saucer", m_impl->web_class);

        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::loadStarted, [this]() { m_impl->is_ready = false; });
        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::loadFinished, [this]() { m_impl->is_ready = true; });
        m_impl->web_view->connect(m_impl->web_view, &QWebEngineView::urlChanged, [this](const QUrl &url) { on_url_changed(url.toString().toStdString()); });

        inject(impl::inject_script, load_time::creation);

        m_impl->web_view->show();
    }

    webview::~webview() = default;

    void webview::on_message(const std::string &message)
    {
        if (message == "js_ready")
        {
            m_impl->is_ready = true;
        }
        else if (message == "js_finished")
        {
            auto script = m_impl->web_view->page()->scripts().findScript("_run_js");
            if (!script.isNull())
            {
                auto code = script.sourceCode().toStdString();
                m_impl->web_view->page()->scripts().remove(script);
            }
        }
    }

    void webview::on_url_changed(const std::string &url)
    {
        m_events.at<web_event::url_changed>().fire(url);
    }

    bool webview::get_dev_tools() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_dev_tools(); });
        }
        return static_cast<bool>(m_impl->dev_view);
    }

    std::string webview::get_url() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([&] { return get_url(); });
        }
        return m_impl->web_view->url().toString().toStdString();
    }

    bool webview::get_transparent() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_transparent(); });
        }
        return m_impl->web_view->testAttribute(Qt::WA_TranslucentBackground);
    }

    bool webview::get_context_menu() const
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this] { return get_context_menu(); });
        }
        return m_impl->web_view->contextMenuPolicy() == Qt::ContextMenuPolicy::DefaultContextMenu;
    }

    void webview::serve_embedded(const std::string &file)
    {
        set_url("saucer://" + file);
    }

    void webview::set_dev_tools(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_dev_tools(enabled); });
        }

        if (enabled)
        {
            m_impl->dev_view = std::make_unique<webview>();
            m_impl->web_view->page()->setDevToolsPage(m_impl->dev_view->m_impl->web_view->page());

            m_impl->dev_view->on<window_event::close>([&] {
                m_impl->dev_view.reset();
                return false;
            });

            m_impl->dev_view->show();
        }
        else
        {
            m_impl->dev_view.reset();
        }
    }

    void webview::set_transparent(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_transparent(enabled); });
        }

        m_impl->web_view->setAttribute(Qt::WA_TranslucentBackground, enabled);
        window::m_impl->window->setAttribute(Qt::WA_TranslucentBackground, enabled);
        m_impl->web_view->page()->setBackgroundColor(enabled ? Qt::transparent : Qt::white);
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_context_menu(enabled); });
        }
        m_impl->web_view->setContextMenuPolicy(enabled ? Qt::ContextMenuPolicy::DefaultContextMenu : Qt::ContextMenuPolicy::NoContextMenu);
    }

    void webview::set_url(const std::string &url)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { return set_url(url); });
        }
        m_impl->web_view->setUrl(QString::fromStdString(url));
    }

    void webview::embed_files(embedded_files &&files)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=]() mutable { return embed_files(std::forward<embedded_files>(files)); });
        }

        m_embedded_files.merge(files);

        if (!m_impl->url_scheme_handler)
        {
            m_impl->url_scheme_handler = new impl::saucer_url_scheme_handler(m_impl->web_view);
            dynamic_cast<impl::saucer_url_scheme_handler *>(m_impl->url_scheme_handler)->set_parent(this);
            m_impl->web_view->page()->profile()->installUrlSchemeHandler("saucer", m_impl->url_scheme_handler);
        }
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

        if (m_impl->url_scheme_handler)
        {
            m_impl->web_view->page()->profile()->removeUrlSchemeHandler(m_impl->url_scheme_handler);
            m_impl->url_scheme_handler->deleteLater();
            m_impl->url_scheme_handler = nullptr;
        }
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { run_java_script(java_script); });
        }

        if (m_impl->is_ready)
        {
            m_impl->web_view->page()->runJavaScript(QString::fromStdString(java_script));
        }
        else
        {
            auto script = m_impl->web_view->page()->scripts().findScript("_run_js");

            if (script.isNull())
            {
                script.setName("_run_js");
                script.setRunsOnSubFrames(false);
                script.setWorldId(QWebEngineScript::MainWorld);
                script.setInjectionPoint(QWebEngineScript::DocumentReady);
                script.setSourceCode("window.saucer.on_message(\"js_finished\");");
            }
            else
            {
                m_impl->web_view->page()->scripts().remove(script);
            }

            script.setSourceCode(QString::fromStdString(java_script) + "\n" + script.sourceCode());
            m_impl->web_view->page()->scripts().insert(script);
        }
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { inject(java_script, load_time); });
        }

        QWebEngineScript script;
        bool found = false;

        switch (load_time)
        {
        case load_time::creation:
            script = m_impl->web_view->page()->scripts().findScript("_creation");
            found = !script.isNull();

            script.setName("_creation");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;
        case load_time::ready:
            script = m_impl->web_view->page()->scripts().findScript("_ready");
            found = !script.isNull();

            script.setName("_ready");
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            break;
        }

        if (!found)
        {
            script.setRunsOnSubFrames(false);
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setSourceCode(QString::fromStdString(java_script));
        }
        else
        {
            m_impl->web_view->page()->scripts().remove(script);
            script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(java_script));
        }

        m_impl->web_view->page()->scripts().insert(script);
    }

    void webview::clear(web_event event)
    {
        m_events.clear(event);
    }

    void webview::unregister(web_event event, std::size_t id)
    {
        m_events.unregister(event, id);
    }

    template <> std::size_t webview::on<web_event::url_changed>(events::get_t<web_event::url_changed> &&callback)
    {
        return m_events.at<web_event::url_changed>().add_callback(std::move(callback));
    }
} // namespace saucer