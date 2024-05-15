#include "webview.hpp"
#include "window.qt.impl.hpp"
#include "webview.qt.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QWebEngineScriptCollection>

namespace saucer
{
    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([java_script, load_time, this] { inject(java_script, load_time); });
        }

        QWebEngineScript script;
        bool found = false;

        auto check_previous = [&](const char *name)
        {
            auto scripts = m_impl->web_view->page()->profile()->scripts()->find(name);

            if (scripts.empty())
            {
                return;
            }

            found  = true;
            script = scripts.front();
        };

        switch (load_time)
        {
        case load_time::creation:
            check_previous("_creation");
            script.setName("_creation");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;

        case load_time::ready:
            check_previous("_ready");
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
            m_impl->web_view->page()->profile()->scripts()->remove(script);
            script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(java_script));
        }

        m_impl->web_view->page()->profile()->scripts()->insert(script);
    }
} // namespace saucer
