#include <QFile>
#include <QMainWindow>
#include <QWebChannel>
#include <QWebEngineScriptCollection>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineView>
#include <webview.hpp>

namespace saucer
{
    class saucer_web_class : public QObject
    {
        // NOLINTNEXTLINE
        Q_OBJECT

      private:
        std::function<void(const std::string &)> &m_callback;

      public:
        saucer_web_class(std::function<void(const std::string &)> &callback) : m_callback(callback) {}

      public slots:
        void on_message(const QString &message)
        {
            if (m_callback)
            {
                m_callback(message.toStdString());
            }
        }
    };

    struct window::impl
    {
        std::unique_ptr<QApplication> application;
        std::unique_ptr<QMainWindow> window;
    };

    struct webview::impl
    {
        std::unique_ptr<saucer_web_class> web_class;
        std::unique_ptr<QWebChannel> web_channel;
        std::shared_ptr<QWebEngineView> web_view;
    };

    webview::~webview() = default;
    webview::webview() : m_impl(std::make_unique<impl>())
    {
        m_impl->web_view = std::make_unique<QWebEngineView>();
        m_impl->web_view->show();

        m_impl->web_view->connect(m_impl->web_view.get(), &QWebEngineView::urlChanged, [this](const QUrl &url) {
            if (m_url_changed_callback)
            {
                m_url_changed_callback(url.toString().toStdString());
            }
        });

        window::m_impl->window->setCentralWidget(m_impl->web_view.get());
    }

    void webview::set_url(const std::string &url)
    {
        m_impl->web_view->load(QString::fromStdString(url));
    }

    std::string webview::get_url() const
    {
        return m_impl->web_view->url().toString().toStdString();
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

    void webview::call(const std::string &java_script)
    {
        m_impl->web_view->page()->runJavaScript(QString::fromStdString(java_script));
    }

    void webview::inject(const std::string &java_script, const load_time_t &load_time)
    {
        // ? Due to Qt not executing the scripts in a proper order we should probably queue up everything that gets executed on document-creation, so
        // ? that we can only register one script to be executed on creation and thus achieve an execution in proper order.
        // * In order to achieve in order loading we will register a script and then append to the source of that script.

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
        case load_time_t::done:
            script = m_impl->web_view->page()->scripts().findScript("_finish");
            found = !script.isNull();

            script.setName("_finish");
            script.setInjectionPoint(QWebEngineScript::Deferred);
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
        m_impl->web_view->page()->scripts().clear();
    }

    void webview::set_message_callback(const message_callback_t &callback)
    {
        m_message_callback = callback;

        if (!m_impl->web_channel)
        {
            m_impl->web_class = std::make_unique<saucer_web_class>(m_message_callback);
            m_impl->web_channel = std::make_unique<QWebChannel>(window::m_impl->window.get());

            m_impl->web_channel->registerObject("saucer", m_impl->web_class.get());
            m_impl->web_view->page()->setWebChannel(m_impl->web_channel.get());

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
                    )js",
                       load_time_t::creation);
            }
            web_channel_api.close();
        }
    }

    void webview::set_url_changed_callback(const url_changed_callback_t &callback)
    {
        m_url_changed_callback = callback;
    }
} // namespace saucer

#include "webview.qt.moc.h"