#include <QApplication>
#include <QBuffer>
#include <QEvent>
#include <QFile>
#include <QMainWindow>
#include <QThread>
#include <QWebChannel>
#include <QWebEngineProfile>
#include <QWebEngineScriptCollection>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>
#include <webview.hpp>

namespace saucer
{
    struct window::impl
    {
        std::shared_ptr<QApplication> application;
        std::unique_ptr<QMainWindow> window;
    };

    struct webview::impl
    {
        // NOLINTNEXTLINE
        class url_scheme_handler;
        class saucer_web_class;

      public:
        std::unique_ptr<url_scheme_handler> resource_handler;
        std::unique_ptr<saucer_web_class> web_class;
        std::unique_ptr<QWebChannel> web_channel;
        std::shared_ptr<QWebEngineView> web_view;
        std::atomic<bool> is_loaded = false;
        bool is_thread_safe() const;
    };

    bool webview::impl::is_thread_safe() const
    {
        return web_view->thread() == QThread::currentThread();
    }

    class safe_call : public QEvent
    {
        std::function<void()> m_func;

      public:
        safe_call(std::function<void()> &&func) : QEvent(QEvent::None), m_func(func) {}
        ~safe_call() override
        {
            m_func();
        }
    };

    class webview::impl::saucer_web_class : public QObject
    {
        // NOLINTNEXTLINE
        Q_OBJECT

      private:
        webview &m_webview;

      public:
        saucer_web_class(webview &webview) : m_webview(webview) {}

      public slots:
        void on_message(const QString &message)
        {
            m_webview.on_message(message.toStdString());
        }
    };

    class webview::impl::url_scheme_handler : public QWebEngineUrlSchemeHandler
    {
      private:
        webview &m_webview;

      public:
        url_scheme_handler(webview &webview) : m_webview(webview) {}

      public:
        void requestStarted(QWebEngineUrlRequestJob *request) override
        {
            auto url = request->requestUrl().toString().toStdString();
            if (url.size() > 9)
            {
                url = url.substr(9);
            }
            else
            {
                request->fail(QWebEngineUrlRequestJob::UrlInvalid);
                return;
            }

            if (m_webview.m_embedded_files.count(url))
            {
                const auto &file = m_webview.m_embedded_files.at(url);

                auto *buffer = new QBuffer;
                buffer->open(QIODevice::WriteOnly);
                buffer->write(reinterpret_cast<const char *>(std::get<2>(file)), static_cast<std::int64_t>(std::get<1>(file)));
                buffer->close();

                connect(request, &QObject::destroyed, buffer, &QObject::deleteLater);
                request->reply(QString::fromStdString(std::get<0>(file)).toUtf8(), buffer);
            }
            else
            {
                request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            }
        }
    };

    webview::~webview() = default;
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        m_impl->web_view = std::make_unique<QWebEngineView>();
        m_impl->web_class = std::make_unique<impl::saucer_web_class>(*this);
        m_impl->web_channel = std::make_unique<QWebChannel>(window::m_impl->window.get());

        m_impl->web_view->page()->setWebChannel(m_impl->web_channel.get());
        m_impl->web_channel->registerObject("saucer", m_impl->web_class.get());

        QFile web_channel_api(":/qtwebchannel/qwebchannel.js");
        if (web_channel_api.open(QIODevice::ReadOnly))
        {
            inject(web_channel_api.readAll().toStdString(), load_time_t::creation);
            inject(R"js(
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
                    )js",
                   load_time_t::creation);
        }
        web_channel_api.close();

        m_impl->web_view->connect(m_impl->web_view.get(), &QWebEngineView::urlChanged, [this](const QUrl &url) {
            if (m_url_changed_callback)
            {
                m_url_changed_callback(url.toString().toStdString());
            }
        });

        m_impl->web_view->connect(m_impl->web_view.get(), &QWebEngineView::loadStarted, [this]() { m_impl->is_loaded = false; });

        m_impl->web_view->show();
        window::m_impl->window->setCentralWidget(m_impl->web_view.get());
    }

    void webview::set_url(const std::string &url)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { set_url(url); }));
            return;
        }

        m_impl->web_view->load(QString::fromStdString(url));
    }

    std::string webview::get_url() const
    {
        return m_impl->web_view->url().toString().toStdString();
    }

    void webview::set_dev_tools([[maybe_unused]] bool enabled)
    {
        //? The QtWebEngine does not provide Dev-Tools. You can only access them from an external chrome window.
    }

    bool webview::get_dev_tools() const
    {
        return false;
    }

    void webview::set_context_menu(bool enabled)
    {
        if (!enabled)
        {
            m_impl->web_view->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        }
        else
        {
            m_impl->web_view->setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
        }
    }

    bool webview::get_context_menu() const
    {
        return m_impl->web_view->contextMenuPolicy() == Qt::ContextMenuPolicy::DefaultContextMenu;
    }

    void webview::embed_files(const embedded_files &files)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { embed_files(files); }));
            return;
        }

        m_embedded_files = files;

        if (!m_impl->resource_handler)
        {
            m_impl->resource_handler = std::make_unique<impl::url_scheme_handler>(*this);
            m_impl->web_view->page()->profile()->installUrlSchemeHandler("saucer", m_impl->resource_handler.get());
        }
    }

    void webview::clear_embedded()
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { clear_embedded(); }));
            return;
        }

        m_embedded_files.clear();

        m_impl->web_view->page()->profile()->removeUrlSchemeHandler(m_impl->resource_handler.get());
        m_impl->resource_handler.reset();
    }

    void webview::serve_embedded(const std::string &file)
    {
        set_url("saucer://" + file);
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { run_java_script(java_script); }));
            return;
        }

        if (m_impl->is_loaded)
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

            script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(java_script));
            m_impl->web_view->page()->scripts().insert(script);
        }
    }

    void webview::inject(const std::string &java_script, const load_time_t &load_time)
    {
        // ? Due to Qt not executing the scripts in a proper order we should probably queue up everything that gets executed on document-creation, so
        // ? that we can only register one script to be executed on creation and thus achieve an execution in proper order.
        // * In order to achieve in order loading we will register a script and then append to the source of that script.
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { inject(java_script, load_time); }));
            return;
        }

        QWebEngineScript script;
        bool found = false;

        switch (load_time)
        {
        case load_time_t::creation:
            script = m_impl->web_view->page()->scripts().findScript("_creation");
            found = !script.isNull();

            script.setName("_creation");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;
        case load_time_t::ready:
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

    void webview::clear_scripts()
    {
        if (!m_impl->is_thread_safe())
        {
            QApplication::postEvent(m_impl->web_view.get(), new safe_call([=]() { clear_scripts(); }));
            return;
        }

        m_impl->web_view->page()->scripts().clear();
    }

    void webview::on_url_changed(const url_changed_callback_t &callback)
    {
        m_url_changed_callback = callback;
    }

    void webview::on_message(const std::string &message)
    {
        if (message == "js_finished")
        {
            auto script = m_impl->web_view->page()->scripts().findScript("_run_js");
            if (!script.isNull())
            {
                m_impl->web_view->page()->scripts().remove(script);
                m_impl->is_loaded = true;
            }
        }
        else if (message == "js_ready")
        {
            m_impl->is_loaded = true;
        }
    }
} // namespace saucer

#include "webview.qt5.moc.h"